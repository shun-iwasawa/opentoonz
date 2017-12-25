#include "openglviewerdraw.h"

//Toonz includes
#include "tapp.h"

//Tnzlib includes
#include "toonz/stage2.h"
#include "toonz/preferences.h"
#include "toonz/tscenehandle.h"
#include "toonz/toonzscene.h"
#include "toonz/sceneproperties.h"
#include "toonz/cleanupparameters.h"
#include "toonz/tcamera.h"

//TnzCore includes
#include "tmsgcore.h"
#include "trop.h"

#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>
#include <QVector>
#include <QVector2D>
#include <QColor>

//=============================================================================
// OpenGLViewerDraw
// "modern" version of ViewerDraw
// shader programs and vbo are shared among viewers
//-----------------------------------------------------------------------------

namespace {
  void execWarning(QString& s) {
    DVGui::MsgBox(DVGui::WARNING, s);
  }
};

// called once and create shader programs
void OpenGLViewerDraw::initialize(){
  // called once
  static bool _initialized = false;
  if (_initialized) return;
  _initialized = true;

  // simple shader
  m_simpleShader.vert = new QOpenGLShader(QOpenGLShader::Vertex);
  const char *simple_vsrc =
    "#version 330 core\n"
    "// Input vertex data, different for all executions of this shader.\n"
    "layout(location = 0) in vec3 vertexPosition; \n"
    "// Output data ; will not be interpolated with flat option. \n"
    "flat out vec4 fragmentColor; \n"
    "// Values that stay constant for the whole mesh.\n"
    "uniform mat4 MVP; \n"
    "uniform vec4 PrimitiveColor; \n"
    "void main() {\n"
    "  // Output position of the vertex, in clip space : MVP * position\n"
    "  gl_Position = MVP * vec4(vertexPosition, 1); \n"
    "  // The color of each vertex will be interpolated\n"
    "  // to produce the color of each fragment\n"
    "  fragmentColor = PrimitiveColor;\n"
    "}\n";
  bool ret = m_simpleShader.vert->compileSourceCode(simple_vsrc);
  if (!ret) execWarning(QObject::tr("Failed to compile m_simpleShader.vert.","gl"));
    
  m_simpleShader.frag = new QOpenGLShader(QOpenGLShader::Fragment);
  const char *simple_fsrc =
    "#version 330 core \n"
    "// non-interpolated values from the vertex shaders \n"
    "flat in vec4 fragmentColor; \n"
    "// Output data \n"
    "out vec4 color; \n"
    "void main() { \n"
    "  // Output color = color specified in the vertex shader \n"
    "  color = fragmentColor; \n"
    "} \n";
  ret = m_simpleShader.frag->compileSourceCode(simple_fsrc);
  if (!ret) execWarning(QObject::tr("Failed to compile m_simpleShader.frag.", "gl"));

  m_simpleShader.program = new QOpenGLShaderProgram();
  //add shaders
  ret = m_simpleShader.program->addShader(m_simpleShader.vert);
  if (!ret) execWarning(QObject::tr("Failed to add m_simpleShader.vert.", "gl"));
  ret = m_simpleShader.program->addShader(m_simpleShader.frag);
  if (!ret) execWarning(QObject::tr("Failed to add m_simpleShader.frag.", "gl"));
  //link shaders
  ret = m_simpleShader.program->link();
  if (!ret) execWarning(QObject::tr("Failed to link simple shader: %1", "gl").arg(m_simpleShader.program->log()));
  //obtain parameter locations
  m_simpleShader.vertexAttrib = m_simpleShader.program->attributeLocation("vertexPosition");
  if(m_simpleShader.vertexAttrib == -1)
    execWarning(QObject::tr("Failed to get attiebute location of vertexPosition", "gl"));
  m_simpleShader.mvpMatrixUniform = m_simpleShader.program->uniformLocation("MVP");
  if (m_simpleShader.vertexAttrib == -1)
    execWarning(QObject::tr("Failed to get uniform location of MVP", "gl"));
  m_simpleShader.colorUniform = m_simpleShader.program->uniformLocation("PrimitiveColor");
  if (m_simpleShader.colorUniform == -1)
    execWarning(QObject::tr("Failed to get uniform location of PrimitiveColor", "gl"));

  //disk
  createDiskVBO();


}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::finalize() {
  if(m_simpleShader.program)
    delete m_simpleShader.program;
  if(m_simpleShader.vert)
    delete m_simpleShader.vert;
  if(m_simpleShader.frag)
    delete m_simpleShader.frag;
  if (m_diskVBO.isCreated())
    m_diskVBO.destroy();
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawDisk(QMatrix4x4& mvp) {
  m_simpleShader.program->bind();
  m_simpleShader.program->setUniformValue(m_simpleShader.mvpMatrixUniform, mvp);
  m_simpleShader.program->enableAttributeArray(m_simpleShader.vertexAttrib);

  //draw disk
  // set color
  //0.6f, 0.65f, 0.7f
  m_simpleShader.program->setUniformValue(m_simpleShader.colorUniform, QColor(153, 166, 179));
  
  m_diskVBO.bind();
  m_simpleShader.program->setAttributeBuffer(m_simpleShader.vertexAttrib, GL_FLOAT, 0, 2);

  m_diskVBO.release();

  glDrawArrays(GL_TRIANGLE_FAN, 0, m_diskVertexOffset[0]);

  //draw line
  // set color
  m_simpleShader.program->setUniformValue(m_simpleShader.colorUniform, QColor(Qt::black));
  glDrawArrays(GL_LINE_LOOP, 0, m_diskVertexOffset[0]);

  //draw inner shape
  // set color
  TPixel32 bgColor;
  if (ToonzCheck::instance()->getChecks() & ToonzCheck::eBlackBg)
    bgColor = TPixel::Black;
  else
    bgColor = Preferences::instance()->getViewerBgColor();
  //GLfloat bgColorF[3] = { (float)bgColor.r / 255.0f, (float)bgColor.g / 255.0f, (float)bgColor.b / 255.0f };
  m_simpleShader.program->setUniformValue(m_simpleShader.colorUniform, QColor(bgColor.r, bgColor.g, bgColor.b));
  glDrawArrays(GL_TRIANGLE_FAN, m_diskVertexOffset[0], m_diskVertexOffset[1] - m_diskVertexOffset[0]);
 
  m_simpleShader.program->disableAttributeArray(m_simpleShader.vertexAttrib);

}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::createDiskVBO() {

  QVector<GLfloat> vertex;
  int vertCount = 0;

  QVector<QPointF> sinCosTable;
  int n = 120;
  sinCosTable.resize(n);
  float ang = 0.0f;
  float d_ang = 2.0f * 3.1415293f / (float)n;
  for (int i = 0; i < n; i++, ang += d_ang) {
    sinCosTable[i].setX(std::cos(ang));
    sinCosTable[i].setY(std::sin(ang));
  }

  float r = 10.0f * (float)Stage::inch;
  // big disk drawn with GL_TRIANGLE_FAN
  // border line drawn with GL_LINE_LOOP (using the same vertex)
  for (int i = 0; i < n; i++) {
    vertex.push_back(r * sinCosTable[i].x());
    vertex.push_back(r * sinCosTable[i].y());
    //vertex.push_back(0.0f);
    vertCount++;
  }

  m_diskVertexOffset[0] = vertCount;

  // inside shape drawn with GL_TRIANGLE_FAN
  r *= 0.9f;
  int m = 13;
  for (int i = n - m; i < n; i++) {
    vertex.push_back(r * sinCosTable[i].x());
    vertex.push_back(r * sinCosTable[i].y());
    //vertex.push_back(0.0f);
    vertCount++;
  }
  for (int i = 0; i <= m; i++) {
    vertex.push_back(r * sinCosTable[i].x());
    vertex.push_back(r * sinCosTable[i].y());
    //vertex.push_back(0.0f);
    vertCount++;
  }
  for (int i = n / 2 - m; i <= n / 2 + m; i++) {
    vertex.push_back(r * sinCosTable[i].x());
    vertex.push_back(r * sinCosTable[i].y());
    //vertex.push_back(0.0f);
    vertCount++;
  }
  m_diskVertexOffset[1] = vertCount;
  
  m_diskVBO.create();
  m_diskVBO.bind();
  m_diskVBO.allocate(vertex.constData(), vertex.count() * sizeof(GLfloat));
  m_diskVBO.release();
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawColorcard(UCHAR channel, QMatrix4x4& mvp) {
  ToonzScene *scene = TApp::instance()->getCurrentScene()->getScene();
  TRectD rect = getCameraRect();

  TPixel color = (ToonzCheck::instance()->getChecks() & ToonzCheck::eBlackBg)
    ? TPixel::Black
    : scene->getProperties()->getBgColor();
  if (channel == 0)
    color.m = 255;  // fondamentale: senno' non si vedono i fill con le texture
                    // in camera stand!
  else {
    if (channel == TRop::MChan) {
      switch (channel) {
      case TRop::RChan:
        color.r = color.g = color.b = color.m = color.r;
        break;
      case TRop::GChan:
        color.r = color.g = color.b = color.m = color.g;
        break;
      case TRop::BChan:
        color.r = color.g = color.b = color.m = color.b;
        break;
      case TRop::MChan:
        color.r = color.g = color.b = color.m = color.m;
        break;
      default:
        assert(false);
      }
    }
    else {
      color.r = channel & TRop::RChan ? color.r : 0;
      color.b = channel & TRop::BChan ? color.b : 0;
      color.g = channel & TRop::GChan ? color.g : 0;
    }
  }

  QVector2D vert[4] = { QVector2D(rect.getP00().x,rect.getP00().y)
    ,QVector2D(rect.getP01().x,rect.getP01().y)
    ,QVector2D(rect.getP11().x,rect.getP11().y)
    ,QVector2D(rect.getP10().x,rect.getP10().y) };
  
  m_simpleShader.program->bind();
  m_simpleShader.program->setUniformValue(m_simpleShader.mvpMatrixUniform, mvp);
  m_simpleShader.program->setUniformValue(m_simpleShader.colorUniform, QColor(color.r, color.g, color.b, color.m));
  m_simpleShader.program->enableAttributeArray(m_simpleShader.vertexAttrib);

  // use vertex array instead of vbo
  m_simpleShader.program->setAttributeArray(m_simpleShader.vertexAttrib, vert);
  
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  m_simpleShader.program->disableAttributeArray(m_simpleShader.vertexAttrib);
}

//-----------------------------------------------------------------------------

TRectD OpenGLViewerDraw::getCameraRect() {
  if (CleanupPreviewCheck::instance()->isEnabled() ||
    CameraTestCheck::instance()->isEnabled())
    return TApp::instance()
    ->getCurrentScene()
    ->getScene()
    ->getProperties()
    ->getCleanupParameters()
    ->m_camera.getStageRect();
  else
    return TApp::instance()
    ->getCurrentScene()
    ->getScene()
    ->getCurrentCamera()
    ->getStageRect();
}

//-----------------------------------------------------------------------------

QMatrix4x4& OpenGLViewerDraw::toQMatrix(const TAffine&aff) {
  return QMatrix4x4((float)aff.a11, (float)aff.a21, 0.0f, 0.0f,
    (float)aff.a12, (float)aff.a22, 0, 0,
    0.0f, 0.0f, 1.0f, 0.0f,
    (float)aff.a13, (float)aff.a23, 0.0f, 1.0f);
}

//-----------------------------------------------------------------------------

TAffine& OpenGLViewerDraw::toTAffine(const QMatrix4x4&matrix) {
  return TAffine(matrix.column(0).x(), matrix.column(0).y(), matrix.column(0).w()
    , matrix.column(1).x(), matrix.column(1).y(), matrix.column(1).w());
}

//-----------------------------------------------------------------------------

OpenGLViewerDraw::~OpenGLViewerDraw() {
  finalize();
}

//-----------------------------------------------------------------------------

OpenGLViewerDraw* OpenGLViewerDraw::instance() {
  static OpenGLViewerDraw instance;
  return &instance;
}