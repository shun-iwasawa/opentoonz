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


//-----------------------------------------------------------------------------
// replacement of tglDraw(const TMeshImage &, const DrawableTextureData &,
// const TAffine &, const PlasticDeformerDataGroup &) in meshutils.cpp
//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawPlasticDeformedImage(
  const TMeshImage &meshImage, const DrawableTextureData &texData,
  const TAffine &meshToTexAff,
  const PlasticDeformerDataGroup &group,
  const double* pixScale,
  const TAffine & aff
) {

  assert((glGetError()) == GL_NO_ERROR);
  auto setVertexValues = [](std::vector<GLdouble>&list, double texCoord_u, double texCoord_v, double vertPos_x, double vertPos_y) {
    list.push_back(texCoord_u);
    list.push_back(texCoord_v);
    list.push_back(vertPos_x);
    list.push_back(vertPos_y);
    return; 
  };

  typedef MeshTexturizer::TextureData::TileData TileData;

  myGlPushAttrib();  // Preserve original status bits

  glEnable(GL_TEXTURE_2D);
  glEnable(GL_BLEND);

  ///glEnable(GL_LINE_SMOOTH);
  ///glLineWidth(1.0f);

  ///glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  // Prepare variables
  const std::vector<TTextureMeshP> &meshes = meshImage.meshes();
  const TTextureMesh *mesh;

  typedef std::vector<std::pair<int, int>> SortedFacesVector;
  const SortedFacesVector &sortedFaces = group.m_sortedFaces;

  const MeshTexturizer::TextureData *td = texData.m_textureData;
  int t, tCount = td->m_tileDatas.size();

  GLuint texId = -1;
  int m = -1;
  const double *dstCoords;

  int v0, v1, v2;
  int e1ovi, e2ovi;  // Edge X's Other Vertex Index (see below)

                     // Prepare each tile's affine
  std::vector<TAffine> tileAff(tCount);
  for (t = 0; t != tCount; ++t) {
    const TileData &tileData = td->m_tileDatas[t];
    const TRectD &tileRect = tileData.m_tileGeometry;

    tileAff[t] = TScale(1.0 / (tileRect.x1 - tileRect.x0),
      1.0 / (tileRect.y1 - tileRect.y0)) *
      TTranslation(-tileRect.x0, -tileRect.y0) * meshToTexAff;
  }

  // Draw each face individually, according to the group's sorted faces list.
  // Change tile and mesh data only if they change - improves performance

  std::vector<GLdouble> vert_list;
  std::vector<GLdouble> edge_list;

  QMatrix4x4 MVPMatrix = m_MVPMatrix * m_modelMatrix * toQMatrix(aff);
  m_textureShader.program->bind();
  m_textureShader.program->setUniformValue(m_textureShader.mvpMatrixUniform, MVPMatrix);
  m_textureShader.program->enableAttributeArray(m_textureShader.vertexAttrib);
  m_textureShader.program->enableAttributeArray(m_textureShader.texCoordAttrib);
  

  SortedFacesVector::const_iterator sft, sfEnd(sortedFaces.end());
  for (sft = sortedFaces.begin(); sft != sfEnd; ++sft) {
    int f = sft->first, m_ = sft->second;

    if (m != m_) {
      // Change mesh if different from current
      m = m_;

      mesh = meshes[m].getPointer();
      dstCoords = group.m_datas[m].m_output.get();
    }

    // Draw each face
    const TTextureMesh::face_type &fc = mesh->face(f);

    const TTextureMesh::edge_type &ed0 = mesh->edge(fc.edge(0)),
      &ed1 = mesh->edge(fc.edge(1)),
      &ed2 = mesh->edge(fc.edge(2));

    {
      v0 = ed0.vertex(0);
      v1 = ed0.vertex(1);
      v2 = ed1.vertex((ed1.vertex(0) == v0) | (ed1.vertex(0) == v1));

      e1ovi = (ed1.vertex(0) == v1) |
        (ed1.vertex(1) == v1);  // ed1 and ed2 will refer to vertexes
      e2ovi = 1 - e1ovi;              // with index 2 and these.
    }

    const TPointD &p0 = mesh->vertex(v0).P(), &p1 = mesh->vertex(v1).P(),
      &p2 = mesh->vertex(v2).P();
    
    for (t = 0; t != tCount; ++t) {
      // Draw face against tile
      const TileData &tileData = td->m_tileDatas[t];

      // Map each face vertex to tile coordinates
      TPointD s[3] = { tileAff[t] * p0, tileAff[t] * p1, tileAff[t] * p2 };

      // Test the face bbox - tile intersection
      if (std::min({ s[0].x, s[1].x, s[2].x }) > 1.0 ||
        std::min({ s[0].y, s[1].y, s[2].y }) > 1.0 ||
        std::max({ s[0].x, s[1].x, s[2].x }) < 0.0 ||
        std::max({ s[0].y, s[1].y, s[2].y }) < 0.0)
        continue;

      // If the tile has changed, interrupt the glBegin/glEnd block and bind the
      // OpenGL texture corresponding to the new tile
      if (tileData.m_textureId != texId) {

        if (texId >= 0 && !vert_list.empty()) {
          glBindTexture( GL_TEXTURE_2D, texId );

          m_textureShader.program->setUniformValue(m_textureShader.texUniform, 0);

          static const int stride = sizeof(GLdouble) * 4;

          if (!edge_list.empty()) {
            m_textureShader.program->setAttributeArray(m_textureShader.texCoordAttrib, GL_DOUBLE, &edge_list[0], 2, stride);
            m_textureShader.program->setAttributeArray(m_textureShader.vertexAttrib, GL_DOUBLE, &edge_list[2], 2, stride);

            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
            glDrawArrays(GL_LINES, 0, edge_list.size()/4);

            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
            glDrawArrays(GL_LINES, 0, edge_list.size() / 4);

            edge_list.clear();
          }

          m_textureShader.program->setAttributeArray(m_textureShader.texCoordAttrib, GL_DOUBLE, &vert_list[0], 2, stride);
          m_textureShader.program->setAttributeArray(m_textureShader.vertexAttrib, GL_DOUBLE, &vert_list[2], 2, stride);

          glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
          glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
          glDrawArrays(GL_TRIANGLES, 0, vert_list.size() / 4);

          glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
          glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
          glDrawArrays(GL_TRIANGLES, 0, vert_list.size() / 4);

          vert_list.clear();
        }


        texId = tileData.m_textureId;


        ///glBindTexture(
        ///  GL_TEXTURE_2D,
        ///  tileData
        ///  .m_textureId);  // This must be OUTSIDE a glBegin/glEnd block
      }

      const double *d[3] = { dstCoords + (v0 << 1), dstCoords + (v1 << 1),
        dstCoords + (v2 << 1) };

      /*
      Now, draw primitives. A note about pixel arithmetic, here.

      Since line antialiasing in OpenGL just manipulates output fragments' alpha
      components,
      we must require that the input texture is NONPREMULTIPLIED.

      Furthermore, this function does not rely on the assumption that the output alpha
      component
      is discarded (as it happens when drawing on screen). This means that just using
      a simple
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) is not an option, since this
      way THE INPUT
      SRC ALPHA GETS MULTIPLIED BY ITSELF - see glBlendFunc's docs - and that shows.

      The solution is to separate the rendering of RGB and M components - the formers
      use
      GL_SRC_ALPHA, while the latter uses GL_ONE. The result is a PREMULTIPLIED image.
      */

      // First, draw antialiased face edges on the mesh border.
      bool drawEd0 = (ed0.facesCount() < 2), drawEd1 = (ed1.facesCount() < 2),
        drawEd2 = (ed2.facesCount() < 2);

      ///glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      ///glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

      ///glBegin(GL_LINES);
      //{
        if (drawEd0) {
          setVertexValues(edge_list, s[0].x, s[0].y, *d[0], *(d[0] + 1));
          setVertexValues(edge_list, s[1].x, s[1].y, *d[1], *(d[1] + 1));
        }

        if (drawEd1) {
          setVertexValues(edge_list, s[e1ovi].x, s[e1ovi].y, *d[e1ovi], *(d[e1ovi] + 1));
          setVertexValues(edge_list, s[2].x, s[2].y, *d[2], *(d[2] + 1));
        }

        if (drawEd2) {
          setVertexValues(edge_list, s[e2ovi].x, s[e2ovi].y, *d[e2ovi], *(d[e2ovi] + 1));
          setVertexValues(edge_list, s[2].x, s[2].y, *d[2], *(d[2] + 1));
        }
      //}
      ///glEnd();
      /*
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

      glBegin(GL_LINES);
      {
        if (drawEd0) {
          glTexCoord2d(s[0].x, s[0].y), glVertex2d(*d[0], *(d[0] + 1));
          glTexCoord2d(s[1].x, s[1].y), glVertex2d(*d[1], *(d[1] + 1));
        }

        if (drawEd1) {
          glTexCoord2d(s[e1ovi].x, s[e1ovi].y),
            glVertex2d(*d[e1ovi], *(d[e1ovi] + 1));
          glTexCoord2d(s[2].x, s[2].y), glVertex2d(*d[2], *(d[2] + 1));
        }

        if (drawEd2) {
          glTexCoord2d(s[e2ovi].x, s[e2ovi].y),
            glVertex2d(*d[e2ovi], *(d[e2ovi] + 1));
          glTexCoord2d(s[2].x, s[2].y), glVertex2d(*d[2], *(d[2] + 1));
        }
      }
      glEnd();
      */

      // Finally, draw the face
      ///glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      ///glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

      ///glBegin(GL_TRIANGLES);
      {
        setVertexValues(vert_list, s[0].x, s[0].y, *d[0], *(d[0] + 1));
        setVertexValues(vert_list, s[1].x, s[1].y, *d[1], *(d[1] + 1));
        setVertexValues(vert_list, s[2].x, s[2].y, *d[2], *(d[2] + 1));
      }
      ///glEnd();
      /*
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);

      glBegin(GL_TRIANGLES);
      {
        glTexCoord2d(s[0].x, s[0].y), glVertex2d(*d[0], *(d[0] + 1));
        glTexCoord2d(s[1].x, s[1].y), glVertex2d(*d[1], *(d[1] + 1));
        glTexCoord2d(s[2].x, s[2].y), glVertex2d(*d[2], *(d[2] + 1));
      }
      glEnd();
      */
    }
  }

  //‚±‚±‚Å‚à•`‰æ
  if (!vert_list.empty()) {

    glBindTexture(GL_TEXTURE_2D, texId);
    m_textureShader.program->setUniformValue(m_textureShader.texUniform, 0);
    //m_textureShader.program->setUniformValue(m_textureShader.texUniform, texId);

    static const int stride = sizeof(GLdouble) * 4;

    if (!edge_list.empty()) {
      m_textureShader.program->setAttributeArray(m_textureShader.texCoordAttrib, GL_DOUBLE, &edge_list[0], 2, stride);
      m_textureShader.program->setAttributeArray(m_textureShader.vertexAttrib, GL_DOUBLE, &edge_list[2], 2, stride);

      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
      glDrawArrays(GL_LINES, 0, edge_list.size() / 4);

      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
      glDrawArrays(GL_LINES, 0, edge_list.size() / 4);
      edge_list.clear();
    }
    
    m_textureShader.program->setAttributeArray(m_textureShader.texCoordAttrib, GL_DOUBLE, &vert_list[0], 2, stride);
    m_textureShader.program->setAttributeArray(m_textureShader.vertexAttrib, GL_DOUBLE, &vert_list[2], 2, stride);
    
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
    glDrawArrays(GL_TRIANGLES, 0, vert_list.size() / 4);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
    glDrawArrays(GL_TRIANGLES, 0, vert_list.size() / 4);
    
    vert_list.clear();
  }
  glDisable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);  // Unbind texture

  m_textureShader.program->disableAttributeArray(m_textureShader.vertexAttrib);
  m_textureShader.program->disableAttributeArray(m_textureShader.texCoordAttrib);

  glDisable(GL_BLEND);

  myGlPopAttrib();
  assert((glGetError()) == GL_NO_ERROR);
}