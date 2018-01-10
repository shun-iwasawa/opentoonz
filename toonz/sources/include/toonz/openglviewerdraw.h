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
class TMeshImage;
struct PlasticDeformerDataGroup;
struct DrawableTextureData;

class DVAPI OpenGLViewerDraw { //singleton
  QMatrix4x4 m_MVPMatrix;
  QMatrix4x4 m_projectionMatrix;
  QMatrix4x4 m_modelMatrix;
  QSize m_vpSize;
  
  struct ShaderBase{
    QOpenGLShader* vert = nullptr;
    QOpenGLShader* frag = nullptr;
    QOpenGLShaderProgram* program = nullptr;
    int mvpMatrixUniform = -1;
    int vertexAttrib = -1;
  };

  // shader program for simple objects
  struct SimpleShader : public ShaderBase{
    int colorUniform = -1;
  }m_simpleShader;

  // shader for shaded objects (having different colors for each vertex)
  struct BasicShader : public ShaderBase{
    int colorAttrib = -1;
  }m_basicShader;

  struct TextureShader : public ShaderBase {
    int texUniform = -1;
    int texCoordAttrib = -1;
  }m_textureShader;

  struct SmoothLineShader : public ShaderBase {
    QOpenGLShader* geom = nullptr;
    int colorUniform = -1;
    int vpSizeUniform = -1;
    int lineWidthUniform = -1;
  }m_smoothLineShader;

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
  void initializeBasicShader();
  void initializeTextureShader();
  void initializeSmoothLineShader();
  // called once in main() on the end
  void finalize();

  void drawDisk();
  void createDiskVBO();

  void drawColorcard(ToonzScene* scene, const UCHAR channel);
  TRectD getCameraRect(ToonzScene* scene);

  void drawSceneRaster(TRaster32P ras);
  void createViewerRasterVBO();

  void setMVPMatrix(QMatrix4x4& mvp);
  QMatrix4x4 getMVPMatrix();

  void setModelMatrix(QMatrix4x4& model);
  QMatrix4x4 getModelMatrix();

  void setViewportSize(QSize& size);
  QSize getViewportSize();

  static QMatrix4x4 toQMatrix(const TAffine&aff);
  static TAffine toTAffine(const QMatrix4x4&matrix);

  static void myGlPushAttrib(GLenum mode = GL_COLOR_BUFFER_BIT);
  static void myGlPopAttrib();

  //vector render (replacement of tvectorgl.h and tglregions.cpp)
private:
  void doDraw(const TVectorImage *vim, const TVectorRenderData &_rd,
    bool drawEnteredGroup);
  void doDrawRegion(const TVectorRenderData &rd, TRegion *r,
    bool pushAttribs);
public:
  void drawVector(const TVectorRenderData &rd, const TVectorImage *vim);
  void drawMaskedVector(const TVectorRenderData &rd, const TVectorImage *vim);
  void drawVector(const TVectorRenderData &rd, const TStroke *stroke,
    bool pushAttribs = true);
  void drawVector(const TVectorRenderData &rd, TRegion *r,
    bool pushAttribs = true);

  //mesh image render (replacement of meshutils.cpp)
private:
  void drawMeshSO(const TMeshImage& mi, const TAffine & aff,
    const PlasticDeformerDataGroup *deformerDatas, bool deformedDomain);
  void drawMeshRigidity(const TMeshImage& mi, const TAffine & aff,
    const PlasticDeformerDataGroup *deformerDatas, bool deformedDomain);
  void drawMeshEdges(const TMeshImage& mi, const TAffine & aff, double opacity,
    const PlasticDeformerDataGroup *deformerDatas);

public:
  void drawMeshImage(const TMeshImage& mi, bool drawSO, bool drawRigidity,
    bool drawMeshes, const TAffine & aff, double opacity, 
    const PlasticDeformerDataGroup *deformerDatas = 0,
    bool deformedDomain = false);

  void drawPlasticDeformedImage(
    const TMeshImage &image,              //!< Mesh image to be drawn.
    const DrawableTextureData &texData,   //!< Textures data to use for texturing.
    const TAffine &meshToTexAffine,       //!< Transform from mesh to texture coordinates.
    const PlasticDeformerDataGroup &deformerDatas,  //!< Data structure of a deformation of the input image.
    const double* pixScale,
    const TAffine & aff
  );

};

#endif
