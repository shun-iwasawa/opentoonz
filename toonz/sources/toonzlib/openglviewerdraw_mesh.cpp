#include "toonz/openglviewerdraw.h"

// TnzExt includes
#include "ext/ttexturesstorage.h"
#include "ext/plasticdeformerstorage.h"

// tcg includes
#include "tcg/tcg_iterator_ops.h"

#include "ext/meshutils.h"


#include <QOpenGLShaderProgram>
#include <QColor>
//********************************************************************************************
//    Colored drawing functions
//********************************************************************************************

namespace {
  struct OpenGLLinearColorFunction {
    typedef double(*ValueFunc)(const OpenGLLinearColorFunction *cf, int m,
      int primitive);

  public:
    const TMeshImage &m_meshImg;
    const PlasticDeformerDataGroup *m_group;

    double m_min, m_max;
    const double *m_cMin, *m_cMax;

    double m_dt;
    bool m_degenerate;

    ValueFunc m_func;

  public:
    OpenGLLinearColorFunction(const TMeshImage &meshImg,
      const PlasticDeformerDataGroup *group, double min,
      double max, const double *cMin, const double *cMax, ValueFunc func)
      : m_meshImg(meshImg)
      , m_group(group)
      , m_min(min)
      , m_max(max)
      , m_cMin(cMin)
      , m_cMax(cMax)
      , m_dt(max - min)
      , m_degenerate(m_dt < 1e-4)
      , m_func(func) {}

    void setVertexColor(int primitive, int m, std::vector<GLdouble>& vert_data){
      if (m_degenerate) {
        for (int c = 0; c < 4; c++)
          vert_data.push_back(0.5 * (m_cMin[c] + m_cMax[c]));
        return;
      }

      double val = m_func(this, m, primitive);
      double t = (val - m_min) / m_dt, one_t = (m_max - val) / m_dt;
      for (int c = 0; c < 4; c++)
        vert_data.push_back(one_t * m_cMin[c] + t * m_cMax[c]);
    }

  };
  

  //-------------------------------------------------------------------------------

  template <typename ColorFunction>
  inline void obtainVertData(const TMeshImage &meshImage,
    ColorFunction colorFunction, std::vector<GLdouble>& vert_data) {
    ///glBegin(GL_TRIANGLES);

    int m, mCount = meshImage.meshes().size();
    for (m = 0; m != mCount; ++m) {
      const TTextureMesh &mesh = *meshImage.meshes()[m];
      const tcg::list<TTextureVertex> &vertices = mesh.vertices();

      // Draw the mesh wireframe
      TTextureMesh::faces_container::const_iterator ft, fEnd = mesh.faces().end();

      for (ft = mesh.faces().begin(); ft != fEnd; ++ft) {
        int v0, v1, v2;
        mesh.faceVertices(ft.index(), v0, v1, v2);

        const TTextureVertex &p0 = vertices[v0];
        const TTextureVertex &p1 = vertices[v1];
        const TTextureVertex &p2 = vertices[v2];

        // store color and vertex position 
        colorFunction.setVertexColor(v0, m, vert_data);
        vert_data.push_back(p0.P().x);
        vert_data.push_back(p0.P().y);
        
        colorFunction.setVertexColor(v1, m, vert_data);
        vert_data.push_back(p1.P().x);
        vert_data.push_back(p1.P().y);

        colorFunction.setVertexColor(v2, m, vert_data);
        vert_data.push_back(p2.P().x);
        vert_data.push_back(p2.P().y);
      }
    }

    ///glEnd();
  }

  //-------------------------------------------------------------------------------

  template <typename ColorFunction>
  inline void obtainVertData(const TMeshImage &meshImage,
    const PlasticDeformerDataGroup *group,
    ColorFunction colorFunction, std::vector<GLdouble>& vert_data) {
    ///glBegin(GL_TRIANGLES);

    // Draw faces according to the group's sorted faces list
    typedef std::vector<std::pair<int, int>> SortedFacesVector;

    const SortedFacesVector &sortedFaces = group->m_sortedFaces;
    const std::vector<TTextureMeshP> &meshes = meshImage.meshes();

    int m = -1;
    const TTextureMesh *mesh;
    const double *dstCoords;

    int v0, v1, v2;

    // Draw each face individually. Change tile and mesh data only if they change
    SortedFacesVector::const_iterator sft, sfEnd(sortedFaces.end());
    for (sft = sortedFaces.begin(); sft != sfEnd; ++sft) {
      int f = sft->first, m_ = sft->second;

      if (m != m_) {
        m = m_;

        mesh = meshes[m].getPointer();
        dstCoords = group->m_datas[m].m_output.get();
      }

      mesh->faceVertices(f, v0, v1, v2);

      const double *d0 = dstCoords + (v0 << 1), *d1 = dstCoords + (v1 << 1),
        *d2 = dstCoords + (v2 << 1);

      // store color and vertex position 
      colorFunction.setVertexColor(v0, m, vert_data);
      vert_data.push_back(*d0);
      vert_data.push_back(*(d0 + 1));

      colorFunction.setVertexColor(v1, m, vert_data);
      vert_data.push_back(*d1);
      vert_data.push_back(*(d1 + 1));

      colorFunction.setVertexColor(v1, m, vert_data);
      vert_data.push_back(*d1);
      vert_data.push_back(*(d1 + 1));
    }

    ///glEnd();
  }
  //-------------------------------------------------------------------------------

  template <typename VerticesContainer, typename PointType>
    inline void obtainMeshEdgePos(const TTextureMesh &mesh,
      const VerticesContainer &vertices, std::vector<GLdouble>&vert_data ) {
    // Draw the mesh wireframe
    ///glBegin(GL_LINES);

    TTextureMesh::edges_container::const_iterator et, eEnd = mesh.edges().end();

    for (et = mesh.edges().begin(); et != eEnd; ++et) {
      const TTextureMesh::edge_type &ed = *et;

      int v0 = ed.vertex(0), v1 = ed.vertex(1);

      const PointType &p0 = vertices[v0];
      const PointType &p1 = vertices[v1];

      // set vertex positions to be rendered with GL_LINES 
      vert_data.push_back(tcg::point_traits<PointType>::x(p0));
      vert_data.push_back(tcg::point_traits<PointType>::y(p0));

      vert_data.push_back(tcg::point_traits<PointType>::x(p1));
      vert_data.push_back(tcg::point_traits<PointType>::y(p1));
    }

    ///glEnd();
  }
}  // namespace


//=============================================================================
// Mesh image rendering part of OpenGLViewerDraw
// It is "modern" OpenGL version of ViewerDraw
// replacement of meshutils.cpp
//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawMeshImage(const TMeshImage& mi, bool drawSO, bool drawRigidity,
  bool drawMeshes, const TAffine & aff, double opacity, const PlasticDeformerDataGroup *deformerData,
  bool deformedDomain) {
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  assert((glGetError()) == GL_NO_ERROR);
  // Draw faces first
  //tglDrawSO
  if (drawSO) drawMeshSO(mi, aff, deformerData, deformedDomain);

  //tglDrawRigidity
  if (drawRigidity) drawMeshRigidity(mi, aff, deformerData, deformedDomain);
  
  // Draw edges next
  if (drawMeshes) drawMeshEdges(mi, aff, opacity, deformerData);
    //glColor4d(0.0, 1.0, 0.0, 0.7 * player.m_opacity / 255.0);  // Green
    //tglDrawEdges(*mi, dataGroup);  // The mesh must be deformed
  
  assert((glGetError()) == GL_NO_ERROR);
  glDisable(GL_BLEND);
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawMeshSO(const TMeshImage& image, const TAffine & aff,
  const PlasticDeformerDataGroup *group, bool deformedDomain) {
  struct locals {
    static double returnSO(const OpenGLLinearColorFunction *cf, int m, int f) {
      return cf->m_group->m_datas[m].m_so[f];
    }
  };

  static const double soMinColor[4] = { 0.0, 0.0, 0.0,
    0.6 };  // Translucent black
  static const double soMaxColor[4] = { 1.0, 1.0, 1.0,
    0.6 };  // Translucent white

  double min = 0.0, max = 0.0;
  if (group) min = group->m_soMin, max = group->m_soMax;

  OpenGLLinearColorFunction colorFunction(image, group, min, max, soMinColor,
    soMaxColor, locals::returnSO);

  std::vector<GLdouble>vert_data;

  if (group && deformedDomain)
    obtainVertData(image, group, colorFunction, vert_data);
  else
    obtainVertData(image, colorFunction, vert_data);

  if (vert_data.empty()) return;

  QMatrix4x4 MVPMatrix = m_MVPMatrix * m_modelMatrix * toQMatrix(aff);

  m_basicShader.program->bind();
  m_basicShader.program->setUniformValue(m_basicShader.mvpMatrixUniform, MVPMatrix);
  m_basicShader.program->enableAttributeArray(m_basicShader.vertexAttrib);
  m_basicShader.program->enableAttributeArray(m_basicShader.colorAttrib);

  static const int stride = sizeof(GLdouble)*(4+2); // r,g,b,a + x,y
  m_basicShader.program->setAttributeArray(m_basicShader.vertexAttrib, GL_DOUBLE, &vert_data.data()[4], 2, stride);
  m_basicShader.program->setAttributeArray(m_basicShader.colorAttrib, GL_DOUBLE, &vert_data.data()[0], 4, stride);
  glDrawArrays(GL_TRIANGLES, 0, vert_data.size()/6);
  
  m_basicShader.program->disableAttributeArray(m_basicShader.vertexAttrib);
  m_basicShader.program->disableAttributeArray(m_basicShader.colorAttrib);

}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawMeshRigidity(const TMeshImage& image, const TAffine & aff,
  const PlasticDeformerDataGroup *group, bool deformedDomain) {
  struct locals {
    static double returnRigidity(const OpenGLLinearColorFunction *cf, int m, int v) {
      return cf->m_meshImg.meshes()[m]->vertex(v).P().rigidity;
    }
  };

  static const double rigMinColor[4] = { 0.0, 1.0, 0.0, 0.6 };  // Translucent green
  static const double rigMaxColor[4] = { 1.0, 0.0, 0.0, 0.6 };  // Translucent
  
  OpenGLLinearColorFunction colorFunction(image, group, 1.0, 1e4, rigMinColor,
    rigMaxColor, locals::returnRigidity);

  std::vector<GLdouble>vert_data;

  if (group && deformedDomain)
    obtainVertData(image, group, colorFunction, vert_data);
  else
    obtainVertData(image, colorFunction, vert_data);

  if (vert_data.empty()) return;

  QMatrix4x4 MVPMatrix = m_MVPMatrix * m_modelMatrix * toQMatrix(aff);

  m_basicShader.program->bind();
  m_basicShader.program->setUniformValue(m_basicShader.mvpMatrixUniform, MVPMatrix);
  m_basicShader.program->enableAttributeArray(m_basicShader.vertexAttrib);
  m_basicShader.program->enableAttributeArray(m_basicShader.colorAttrib);

  static const int stride = sizeof(GLdouble)*(4 + 2); // r,g,b,a + x,y
  m_basicShader.program->setAttributeArray(m_basicShader.vertexAttrib, GL_DOUBLE, &vert_data.data()[4], 2, stride);
  m_basicShader.program->setAttributeArray(m_basicShader.colorAttrib, GL_DOUBLE, &vert_data.data()[0], 4, stride);
  glDrawArrays(GL_TRIANGLES, 0, vert_data.size() / 6);

  m_basicShader.program->disableAttributeArray(m_basicShader.vertexAttrib);
  m_basicShader.program->disableAttributeArray(m_basicShader.colorAttrib);

}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawMeshEdges(const TMeshImage& mi, const TAffine & aff,
  double opacity, const PlasticDeformerDataGroup *group) {
  const std::vector<TTextureMeshP> &meshes = mi.meshes();

  std::vector<GLdouble>vert_data;
  int m, mCount = meshes.size();
  if (group) {
    for (m = 0; m != mCount; ++m)
      obtainMeshEdgePos<const TPointD *, TPointD>(
        *meshes[m], (const TPointD *)group->m_datas[m].m_output.get(), vert_data);
  }
  else {
    for (m = 0; m != mCount; ++m) {
      const TTextureMesh &mesh = *meshes[m];
      obtainMeshEdgePos<tcg::list<TTextureMesh::vertex_type>, TTextureVertex>(
        mesh, mesh.vertices(), vert_data);
    }
  }

  if (vert_data.empty()) return;

  QMatrix4x4 MVPMatrix = m_MVPMatrix * m_modelMatrix * toQMatrix(aff);

  QColor lineColor(Qt::green);
  lineColor.setAlphaF(0.7 * opacity);  // Green
  
  m_smoothLineShader.program->bind();
  m_smoothLineShader.program->setUniformValue(m_smoothLineShader.mvpMatrixUniform, MVPMatrix);
  m_smoothLineShader.program->setUniformValue(m_smoothLineShader.vpSizeUniform, m_vpSize);
  m_smoothLineShader.program->enableAttributeArray(m_smoothLineShader.vertexAttrib);
  m_smoothLineShader.program->setUniformValue(m_smoothLineShader.colorUniform, lineColor);
  m_smoothLineShader.program->setUniformValue(m_smoothLineShader.lineWidthUniform, 1.0f);

  m_smoothLineShader.program->setAttributeArray(m_smoothLineShader.vertexAttrib, GL_DOUBLE, &vert_data.data()[0], 2);
  glDrawArrays(GL_LINES, 0, vert_data.size() / 2);

  m_smoothLineShader.program->disableAttributeArray(m_smoothLineShader.vertexAttrib);

}