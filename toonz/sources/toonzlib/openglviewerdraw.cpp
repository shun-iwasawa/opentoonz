#include "toonz/openglviewerdraw.h"

//Tnzlib includes
#include "toonz/stage2.h"
#include "toonz/preferences.h"
#include "toonz/tscenehandle.h"
#include "toonz/toonzscene.h"
#include "toonz/sceneproperties.h"
#include "toonz/cleanupparameters.h"
#include "toonz/tcamera.h"
#include "toonz/toonzfolders.h"

//TnzCore includes
#include "tmsgcore.h"
#include "trop.h"
#include "tgl.h"

#include <QOpenGLShader>
#include <QOpenGLShaderProgram>
#include <QOpenGLContext>
#include <QVector>
#include <QVector2D>
#include <QColor>
#include <QOpenGLTexture>
#include <QStack>

//=============================================================================
// OpenGLViewerDraw
// "modern" version of ViewerDraw
// shader programs and vbo are shared among viewers
//-----------------------------------------------------------------------------

namespace {

  QList<GLenum> gl_enable_bit_list =
  { GL_ALPHA_TEST,
    GL_BLEND,
    GL_DITHER,
    GL_COLOR_LOGIC_OP,
    GL_INDEX_LOGIC_OP,
    //----GL_COLOR_BUFFER_BIT----
    GL_AUTO_NORMAL,
    GL_COLOR_MATERIAL,
    GL_CULL_FACE,
    GL_DEPTH_TEST,
    GL_FOG,
    //GL_LIGHTi(0 <= i<GL_MAX_LIGHTS)
    GL_LIGHTING,
    GL_LINE_SMOOTH,
    GL_LINE_STIPPLE,
    //GL_MAP1_x(x �̓}�b�v�̌^)
    //GL_MAP2_x(x �̓}�b�v�̌^)
    GL_NORMALIZE,
    GL_POINT_SMOOTH,
    GL_POLYGON_OFFSET_LINE,
    GL_POLYGON_OFFSET_FILL,
    GL_POLYGON_OFFSET_POINT,
    GL_POLYGON_SMOOTH,
    GL_POLYGON_STIPPLE,
    GL_SCISSOR_TEST,
    GL_STENCIL_TEST,
    GL_TEXTURE_1D,
    GL_TEXTURE_2D,
    GL_TEXTURE_3D
    //GL_TEXTURE_GEN_x,�Q(x �� S, T, R, Q �̂����ꂩ) 
  };

  void execWarning(QString& s) {
    DVGui::MsgBox(DVGui::WARNING, s);
  }
  
  QStack<QMap<GLenum, GLboolean>> attribStack;
  void setGLEnabled(GLenum pname, GLboolean enable) {
    if (enable)
      glEnable(pname);
    else
      glDisable(pname);
  }
};

// called once and create shader programs
void OpenGLViewerDraw::initialize(){
  // called once
  static bool _initialized = false;
  if (_initialized) return;
  _initialized = true;

  // simple shader
  initializeSimpleShader();
  //texture shader
  initializeTextureShader();
  // smooth shader
  initializeSmoothShader();

  //disk
  createDiskVBO();
  //viewer raster
  createViewerRasterVBO();

  m_viewerRasterTex = new QOpenGLTexture(QOpenGLTexture::Target2D);
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::initializeSimpleShader() {
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
    "  // The color of each vertex \n"
    "  fragmentColor = PrimitiveColor;\n"
    "}\n";
  bool ret = m_simpleShader.vert->compileSourceCode(simple_vsrc);
  if (!ret) execWarning(QObject::tr("Failed to compile m_simpleShader.vert.", "gl"));

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
  if (m_simpleShader.vertexAttrib == -1)
    execWarning(QObject::tr("Failed to get attribute location of vertexPosition", "gl"));
  m_simpleShader.mvpMatrixUniform = m_simpleShader.program->uniformLocation("MVP");
  if (m_simpleShader.vertexAttrib == -1)
    execWarning(QObject::tr("Failed to get uniform location of MVP", "gl"));
  m_simpleShader.colorUniform = m_simpleShader.program->uniformLocation("PrimitiveColor");
  if (m_simpleShader.colorUniform == -1)
    execWarning(QObject::tr("Failed to get uniform location of PrimitiveColor", "gl"));
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::initializeTextureShader() {
  m_textureShader.vert = new QOpenGLShader(QOpenGLShader::Vertex);
  const char *simple_vsrc =
    "#version 330 core\n"
    "// Input vertex data, different for all executions of this shader.\n"
    "layout(location = 0) in vec3 vertexPosition;\n"
    "layout(location = 1) in vec2 texCoord;\n"
    "// Output data ; will be interpolated for each fragment.\n"
    "out vec2 UV;\n"
    "// Values that stay constant for the whole mesh.\n"
    "uniform mat4 MVP;\n"
    "void main() {\n"
    "  // Output position of the vertex, in clip space : MVP * position\n"
    "  gl_Position = MVP * vec4(vertexPosition, 1);\n"
    "  // UV of the vertex. No special space for this one.\n"
    "  UV = texCoord;\n"
    "}\n";
  bool ret = m_textureShader.vert->compileSourceCode(simple_vsrc);
  if (!ret) execWarning(QObject::tr("Failed to compile m_textureShader.vert.", "gl"));

  m_textureShader.frag = new QOpenGLShader(QOpenGLShader::Fragment);
  const char *simple_fsrc =
    "#version 330 core \n"
    "// Interpolated values from the vertex shaders \n"
    "in vec2 UV; \n"
    "// Ouput data \n"
    "out vec4 color; \n"
    "// Values that stay constant for the whole mesh. \n"
    "uniform sampler2D tex; \n"
    "void main() { \n"
    "  // Output color = color of the texture at the specified UV \n"
    "  color = texture(tex, UV); \n"
    "} \n";
  ret = m_textureShader.frag->compileSourceCode(simple_fsrc);
  if (!ret) execWarning(QObject::tr("Failed to compile m_textureShader.frag.", "gl"));

  m_textureShader.program = new QOpenGLShaderProgram();
  //add shaders
  ret = m_textureShader.program->addShader(m_textureShader.vert);
  if (!ret) execWarning(QObject::tr("Failed to add m_textureShader.vert.", "gl"));
  ret = m_textureShader.program->addShader(m_textureShader.frag);
  if (!ret) execWarning(QObject::tr("Failed to add m_textureShader.frag.", "gl"));
  //link shaders
  ret = m_textureShader.program->link();
  if (!ret) execWarning(QObject::tr("Failed to link simple shader: %1", "gl").arg(m_textureShader.program->log()));
  //obtain parameter locations
  m_textureShader.vertexAttrib = m_textureShader.program->attributeLocation("vertexPosition");
  if (m_textureShader.vertexAttrib == -1)
    execWarning(QObject::tr("Failed to get attribute location of vertexPosition", "gl"));
  m_textureShader.texCoordAttrib = m_textureShader.program->attributeLocation("texCoord");
  if (m_textureShader.texCoordAttrib == -1)
    execWarning(QObject::tr("Failed to get attribute location of texCoord", "gl"));
  m_textureShader.mvpMatrixUniform = m_textureShader.program->uniformLocation("MVP");
  if (m_textureShader.vertexAttrib == -1)
    execWarning(QObject::tr("Failed to get uniform location of MVP", "gl"));
  m_textureShader.texUniform = m_textureShader.program->uniformLocation("tex");
  if (m_textureShader.texUniform == -1)
    execWarning(QObject::tr("Failed to get uniform location of tex", "gl"));
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::initializeSmoothShader() {
  m_smoothShader.vert = new QOpenGLShader(QOpenGLShader::Vertex);
  const char *smooth_vsrc =
    "#version 330 core"
    "// Input vertex data, different for all executions of this shader.\n"
    "layout(location = 0) in vec3 vertexPosition; \n"
    "// Values that stay constant for the whole mesh.\n"
    "uniform mat4 MVP; \n"
    "uniform vec4 PrimitiveColor; \n"
    "out vec4 vertColor; \n"
    "void main() \n"
    "{ \n"
    "  vertColor = PrimitiveColor; \n"
    "  gl_Position = MVP * vec4(vertexPosition, 1);\n"
    "} \n";
  bool ret = m_smoothShader.vert->compileSourceCode(smooth_vsrc);
  if (!ret) execWarning(QObject::tr("Failed to compile m_smoothShader.vert.", "gl"));

  m_smoothShader.frag = new QOpenGLShader(QOpenGLShader::Fragment);
  const char *smooth_fsrc =
    "#version 330 core \n"
    "// non-interpolated values from the vertex shaders \n"
    "in vec4 fragColor; \n"
    "// Output data \n"
    "out vec4 color; \n"
    "void main() { \n"
    "  // Output color = color specified in the vertex shader \n"
    "  color = fragColor; \n"
    "} \n";
  ret = m_smoothShader.frag->compileSourceCode(smooth_fsrc);
  if (!ret) execWarning(QObject::tr("Failed to compile m_smoothShader.frag.", "gl"));

  m_smoothShader.geom = new QOpenGLShader(QOpenGLShader::Geometry);
  TFilePath fp = ToonzFolder::getLibraryFolder() + TFilePath("shaders/programs/openglviewerdraw_smoothshader.geom");
  ret = m_smoothShader.geom->compileSourceFile(fp.getQString());
  if (!ret) execWarning(QObject::tr("Failed to compile m_smoothShader.geom.", "gl"));

  m_smoothShader.program = new QOpenGLShaderProgram();
  //add shaders
  ret = m_smoothShader.program->addShader(m_smoothShader.vert);
  if (!ret) execWarning(QObject::tr("Failed to add m_smoothShader.vert.", "gl"));
  ret = m_smoothShader.program->addShader(m_smoothShader.frag);
  if (!ret) execWarning(QObject::tr("Failed to add m_smoothShader.frag.", "gl"));
  ret = m_smoothShader.program->addShader(m_smoothShader.geom);
  if (!ret) execWarning(QObject::tr("Failed to add m_smoothShader.geom.", "gl"));
  //link shaders
  ret = m_smoothShader.program->link();
  if (!ret) execWarning(QObject::tr("Failed to link smooth shader: %1", "gl").arg(m_smoothShader.program->log()));
  //obtain parameter locations
  m_smoothShader.vertexAttrib = m_smoothShader.program->attributeLocation("vertexPosition");
  if (m_smoothShader.vertexAttrib == -1)
    execWarning(QObject::tr("Failed to get attribute location of vertexPosition", "gl"));
  m_smoothShader.mvpMatrixUniform = m_smoothShader.program->uniformLocation("MVP");
  if (m_smoothShader.vertexAttrib == -1)
    execWarning(QObject::tr("Failed to get uniform location of MVP", "gl"));
  m_smoothShader.colorUniform = m_smoothShader.program->uniformLocation("PrimitiveColor");
  if (m_smoothShader.colorUniform == -1)
    execWarning(QObject::tr("Failed to get uniform location of PrimitiveColor", "gl"));
  assert(glGetError() == 0);
}
//-----------------------------------------------------------------------------

void OpenGLViewerDraw::finalize() {
  // release simple shader
  if(m_simpleShader.program)
    delete m_simpleShader.program;
  if(m_simpleShader.vert)
    delete m_simpleShader.vert;
  if(m_simpleShader.frag)
    delete m_simpleShader.frag;
  // release texture shader
  if (m_textureShader.program)
    delete m_textureShader.program;
  if (m_textureShader.vert)
    delete m_textureShader.vert;
  if (m_textureShader.frag)
    delete m_textureShader.frag;
  // release vbo
  if (m_diskVBO.isCreated())
    m_diskVBO.destroy();
  // release texture
  if (m_viewerRasterTex->isCreated())
    m_viewerRasterTex->destroy();
  delete m_viewerRasterTex;

}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawDisk() {
  m_simpleShader.program->bind();
  m_simpleShader.program->setUniformValue(m_simpleShader.mvpMatrixUniform, m_MVPMatrix);
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

void OpenGLViewerDraw::drawColorcard(ToonzScene* scene, const UCHAR channel) {
  TRectD rect = getCameraRect(scene);

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

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  m_simpleShader.program->bind();
  m_simpleShader.program->setUniformValue(m_simpleShader.mvpMatrixUniform, m_MVPMatrix);
  m_simpleShader.program->setUniformValue(m_simpleShader.colorUniform, QColor(color.r, color.g, color.b, color.m));
  m_simpleShader.program->enableAttributeArray(m_simpleShader.vertexAttrib);

  // use vertex array instead of vbo
  m_simpleShader.program->setAttributeArray(m_simpleShader.vertexAttrib, vert);
  
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  m_simpleShader.program->disableAttributeArray(m_simpleShader.vertexAttrib);
  glDisable(GL_BLEND);
}

//-----------------------------------------------------------------------------

TRectD OpenGLViewerDraw::getCameraRect(ToonzScene* scene) {
  if (CleanupPreviewCheck::instance()->isEnabled() ||
    CameraTestCheck::instance()->isEnabled())
    return scene
    ->getProperties()
    ->getCleanupParameters()
    ->m_camera.getStageRect();
  else
    return scene
    ->getCurrentCamera()
    ->getStageRect();
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawSceneRaster(TRaster32P ras) {

  assert((glGetError()) == GL_NO_ERROR);

  if (m_viewerRasterTex->isCreated())
    m_viewerRasterTex->destroy();
  m_viewerRasterTex->create();
  
  //QImage red(1, 1, QImage::Format_RGBA8888);
  //red.fill(Qt::red);

  //QImage img(ras->getRawData(), ras->getLx(), ras->getLy(), QImage::Format_RGBA8888);

  //m_viewerRasterTex->setSize(1,1);
  m_viewerRasterTex->setSize(ras->getLx(), ras->getLy());
  m_viewerRasterTex->setFormat(QOpenGLTexture::RGBA8_UNorm);
  m_viewerRasterTex->allocateStorage();
  //unpack alignment���S�����ǁA���܂��������炵�߂�����
  //m_viewerRasterTex->setData(img);
  m_viewerRasterTex->setData((QOpenGLTexture::PixelFormat)TGL_FMT, (QOpenGLTexture::PixelType)TGL_TYPE, ras->getRawData());
    
  m_viewerRasterTex->bind();
    
  m_textureShader.program->bind();
  m_textureShader.program->setUniformValue(m_textureShader.mvpMatrixUniform, m_MVPMatrix);
  m_textureShader.program->setUniformValue(m_textureShader.texUniform, 0); // use texture unit 0

  m_textureShader.program->enableAttributeArray(m_textureShader.vertexAttrib);
  m_textureShader.program->enableAttributeArray(m_textureShader.texCoordAttrib);

  m_viewerRasterVBO.bind();
  m_textureShader.program->setAttributeBuffer(m_textureShader.vertexAttrib, GL_FLOAT, 0, 2);
  m_textureShader.program->setAttributeBuffer(m_textureShader.texCoordAttrib, GL_FLOAT, 4 * 2 * sizeof(GLfloat), 2);

  m_viewerRasterVBO.release();

  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

  m_textureShader.program->disableAttributeArray(m_textureShader.vertexAttrib);
  m_textureShader.program->disableAttributeArray(m_textureShader.texCoordAttrib);

  assert((glGetError()) == GL_NO_ERROR);
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::createViewerRasterVBO() {

  GLfloat vertex[] = {0.0f, 0.0f,  1.0f, 0.0f,
                      1.0f, 1.0f,  0.0f, 1.0f};
  GLfloat texCoord[] = { 0.0f, 0.0f,  1.0f, 0.0f,
                      1.0f, 1.0f,  0.0f, 1.0f };
  

  m_viewerRasterVBO.create();
  m_viewerRasterVBO.bind();
  m_viewerRasterVBO.allocate(4 * 4 * sizeof(GLfloat));
  m_viewerRasterVBO.write(0, vertex, sizeof(vertex));
  m_viewerRasterVBO.write(sizeof(vertex), texCoord, sizeof(texCoord));
  m_viewerRasterVBO.release();
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::setMVPMatrix(QMatrix4x4& mvp) {
  m_MVPMatrix = mvp;
}

//-----------------------------------------------------------------------------

QMatrix4x4& OpenGLViewerDraw::getMVPMatrix() {
  return m_MVPMatrix;
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::setModelMatrix(QMatrix4x4& model) {
  m_modelMatrix = model;
}

//-----------------------------------------------------------------------------

QMatrix4x4& OpenGLViewerDraw::getModelMatrix() {
  return m_modelMatrix;
}

//-----------------------------------------------------------------------------

QMatrix4x4 OpenGLViewerDraw::toQMatrix(const TAffine&aff) {
  QMatrix mat(aff.a11, aff.a21, aff.a12, aff.a22, aff.a13, aff.a23);
  QMatrix4x4 qmat(mat);
  qmat.optimize();
  return qmat;
}

//-----------------------------------------------------------------------------

TAffine OpenGLViewerDraw::toTAffine(const QMatrix4x4&matrix) {
  QMatrix aff = matrix.toAffine();
  return TAffine(aff.m11(), aff.m21(), aff.dx(), aff.m12(), aff.m22(), aff.dy());
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::myGlPushAttrib(GLenum mode) {
  QMap<GLenum, GLboolean> attribList;
  // mode can be GL_COLOR_BUFFER_BIT or GL_ENABLE_BIT
  int attribCount = (mode == GL_COLOR_BUFFER_BIT) ? 5 : gl_enable_bit_list.size();
  for(int a = 0; a < attribCount; a++){
    attribList[gl_enable_bit_list.at(a)] = glIsEnabled(gl_enable_bit_list.at(a));
  }
  attribStack.push(attribList);
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::myGlPopAttrib() {
  QMap<GLenum, GLboolean> attribList = attribStack.pop();
  QMapIterator<GLenum, GLboolean> i(attribList);
  while (i.hasNext()) {
    i.next();
    setGLEnabled(i.key(), i.value());
  }
}
OpenGLViewerDraw::OpenGLViewerDraw() {}

//-----------------------------------------------------------------------------

OpenGLViewerDraw::~OpenGLViewerDraw() {
  finalize();
}

//-----------------------------------------------------------------------------

OpenGLViewerDraw* OpenGLViewerDraw::instance() {
  static OpenGLViewerDraw instance;
  return &instance;
}