#pragma once

#ifndef OPENGL_VIEWERDRAW_INCLUDED
#define OPENGL_VIEWERDRAW_INCLUDED

#undef DVAPI
#undef DVVAR
#ifdef TAPPTOOLS_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif
//=============================================================================
// OpenGLViewerDraw
// "modern" version of ViewerDraw
// shader programs and vbo are shared among viewers
//-----------------------------------------------------------------------------

#include "tgeometry.h"
#include "traster.h"

#include <QOpenGLBuffer>
#include <QMatrix4x4>

class QOpenGLShader;
class QOpenGLShaderProgram;
class QOpenGLTexture;
class ToonzScene;
class TStroke;
class TRegion;
class TVectorImage;
class TVectorRenderData;

class DVAPI OpenGLViewerDraw { //singleton
  QMatrix4x4 m_MVPMatrix;
  QMatrix4x4 m_projectionMatrix;
  QMatrix4x4 m_modelMatrix;

  struct SimpleShader{
    // shader for simple objects
    QOpenGLShader* vert = nullptr;
    QOpenGLShader* frag = nullptr;
    // shader program for simple objects
    QOpenGLShaderProgram* program = nullptr;
    int mvpMatrixUniform = -1;
    int colorUniform = -1;
    int vertexAttrib = -1;
  }m_simpleShader;

  struct TextureShader {
    // shader for raster image
    QOpenGLShader* vert = nullptr;
    QOpenGLShader* frag = nullptr;
    // shader program for raster image
    QOpenGLShaderProgram* program = nullptr;
    int mvpMatrixUniform = -1;
    int texUniform = -1;
    int texCoordAttrib = -1;
    int vertexAttrib = -1;
  }m_textureShader;

  struct SmoothShader {
    // shader for simple objects
    QOpenGLShader* vert = nullptr;
    QOpenGLShader* frag = nullptr;
    QOpenGLShader* geom = nullptr;
    // shader program for simple objects
    QOpenGLShaderProgram* program = nullptr;
    int mvpMatrixUniform = -1;
    int colorUniform = -1;
    int vertexAttrib = -1;
  }m_smoothShader;

  //disk vbo
  QOpenGLBuffer m_diskVBO;
  int m_diskVertexOffset[2];

  //viewer raster image
  QOpenGLBuffer m_viewerRasterVBO;
  QOpenGLTexture* m_viewerRasterTex;
  
  OpenGLViewerDraw();
  ~OpenGLViewerDraw();

public:
  static OpenGLViewerDraw *instance();

  // called once and create shader programs
  void initialize();
  void initializeSimpleShader();
  void initializeTextureShader();
  void initializeSmoothShader();
  // called once in main() on the end
  void finalize();

  void drawDisk();
  void createDiskVBO();

  void drawColorcard(ToonzScene* scene, const UCHAR channel);
  TRectD getCameraRect(ToonzScene* scene);

  void drawSceneRaster(TRaster32P ras);
  void createViewerRasterVBO();

  void setMVPMatrix(QMatrix4x4& mvp);
  QMatrix4x4& getMVPMatrix();

  void setModelMatrix(QMatrix4x4& model);
  QMatrix4x4& getModelMatrix();

  static QMatrix4x4 toQMatrix(const TAffine&aff);
  static TAffine toTAffine(const QMatrix4x4&matrix);

  static void myGlPushAttrib(GLenum mode = GL_COLOR_BUFFER_BIT);
  static void myGlPopAttrib();

  //vector render (replacement of tvectorgl.h and tglregions.cpp)
private:
  void doDraw(const TVectorImage *vim, const TVectorRenderData &_rd,
    bool drawEnteredGroup);
public:
  void drawVector(const TVectorRenderData &rd, const TVectorImage *vim);
  void drawMaskedVector(const TVectorRenderData &rd, const TVectorImage *vim);
  void drawVector(const TVectorRenderData &rd, const TStroke *stroke,
    bool pushAttribs = true);
  void drawVector(const TVectorRenderData &rd, TRegion *r,
    bool pushAttribs = true);
};

#endif
