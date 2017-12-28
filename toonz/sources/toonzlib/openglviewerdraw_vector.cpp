#include "toonz/openglviewerdraw.h"

#include "tvectorimage.h"
#include "tvectorrenderdata.h"
#include "tcolorfunctions.h"
#include "tstrokeprop.h"
#include "tpalette.h"

#include <QMutexLocker>
#include <QOpenGLShaderProgram>
#include <QColor>

//------------------------------------------------------------------------------------

namespace {
  static int Index = 0;

  bool isOThick(const TStroke *s) {
    int i;
    for (i = 0; i < s->getControlPointCount(); i++)
      if (s->getControlPoint(i).thick != 0) return false;
    return true;
  }

  inline QColor & toQColor(TPixel32 & pix) {
    return QColor(pix.r, pix.g, pix.b, pix.m);
  }

  // for debug purpose
  void printMatrix(const QMatrix4x4& m) {
    for (int j = 0; j < 4; j++) {
      for (int i = 0; i < 4; i++) {
        std::cout << m.row(j)[i] << ", ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}//namespace

//=============================================================================
// Vector rendering part of OpenGLViewerDraw
// It is "modern" OpenGL version of ViewerDraw
// shader programs and vbo are shared among viewers
//-----------------------------------------------------------------------------

void OpenGLViewerDraw::doDraw(const TVectorImage *vim, const TVectorRenderData &_rd,
  bool drawEnteredGroup) {
  static TOnionFader *fade = new TOnionFader(TPixel::White, 0.5);

  TVectorRenderData rd(_rd);

  if (!rd.m_palette) {
    TPalette *vPalette = vim->getPalette();
    rd.m_palette = vPalette;
    if (!vPalette) return;
  }

  if (!drawEnteredGroup && !rd.m_isIcon && vim->isInsideGroup() > 0)
    rd.m_cf = fade;

  TVectorRenderData rdRegions = rd;

  UINT strokeIndex = 0;
  Index = 0;

  while (strokeIndex <
    vim->getStrokeCount())  // ogni ciclo di while disegna un gruppo
  {
    int currStrokeIndex = strokeIndex;
    if (!rd.m_isIcon && vim->isInsideGroup() > 0 &&
      ((drawEnteredGroup && !vim->isEnteredGroupStroke(strokeIndex)) ||
        !drawEnteredGroup && vim->isEnteredGroupStroke(strokeIndex))) {
      while (strokeIndex < vim->getStrokeCount() &&
        vim->sameGroup(strokeIndex, currStrokeIndex))
        strokeIndex++;
      continue;
    }
    //draw regions
    if (rd.m_drawRegions)
      for (UINT regionIndex = 0; regionIndex < vim->getRegionCount();
        regionIndex++)
        if (vim->sameGroupStrokeAndRegion(currStrokeIndex, regionIndex))
          drawVector(rdRegions, vim->getRegion(regionIndex));
    //draw strokes
    while (strokeIndex < vim->getStrokeCount() &&
      vim->sameGroup(strokeIndex, currStrokeIndex)) {
      if (rd.m_indexToHighlight != strokeIndex) {
        rd.m_highLightNow = false;
      }
      else {
        rd.m_highLightNow = true;
      }
#if DISEGNO_OUTLINE == 1
      CurrStrokeIndex = strokeIndex;
      CurrVimg = vim;
#endif
      drawVector(rd, vim->getStroke(strokeIndex));
      strokeIndex++;
    }
  }
}

// replacement of tglDraw()
void OpenGLViewerDraw::drawVector(const TVectorRenderData &rd, const TVectorImage *vim) {
  assert(vim);
  if (!vim) return;

  QMutexLocker sl(vim->getMutex());

  myGlPushAttrib(GL_ALL_ATTRIB_BITS);
  glEnable(GL_ALPHA_TEST);
  // deplecated. アルファチャンネルが0より大きいものが描画される
  //glAlphaFunc(GL_GREATER, 0);

  doDraw(vim, rd, false);
  if (!rd.m_isIcon && vim->isInsideGroup() > 0) doDraw(vim, rd, true);

  //glDisable(GL_ALPHA_TEST); //popAttribでやるから大丈夫
  myGlPopAttrib();

#ifdef _DEBUG
  vim->drawAutocloses(rd);
#endif
}

//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawMaskedVector(const TVectorRenderData &rd, const TVectorImage *vim) {

}
//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawVector(const TVectorRenderData &rd, const TStroke *s,
  bool pushAttribs) {
  assert(s);
  if (!s) return;

  assert((glGetError()) == GL_NO_ERROR);
  TStrokeProp *prop = 0;
  bool pushedAttribs = false;

  try {
    TColorStyleP style;
    TStroke *stroke = const_cast<TStroke *>(s);
    if (rd.m_inkCheckEnabled && s->getStyle() == rd.m_colorCheckIndex) {
      static TSolidColorStyle *redColor = new TSolidColorStyle();
      redColor->addRef();
      redColor->setMainColor(TPixel::Red);
      style = redColor;
    }
    else if (rd.m_tcheckEnabled) {
      static TSolidColorStyle *color = new TSolidColorStyle();
      color->addRef();
      color->setMainColor(rd.m_tCheckInk);
      style = color;
    }
    else
      style = rd.m_palette->getStyle(stroke->getStyle());

    if (!rd.m_show0ThickStrokes && isOThick(s) &&
      dynamic_cast<TSolidColorStyle *>(
        style.getPointer())  // This is probably to exclude
                             // TCenterlineStrokeStyle-like styles
      && !rd.m_tcheckEnabled)  // I wonder why this?
      return;

    // const TStroke& stroke = *s;  //serve???

    assert(rd.m_palette);

    prop = s->getProp(/*rd.m_palette*/);
    /////questo codice stava dentro tstroke::getprop/////////
    if (prop) prop->getMutex()->lock();

    if (!style->isStrokeStyle() || style->isEnabled() == false) {
      if (prop) prop->getMutex()->unlock();

      prop = 0;
    }
    else {
      // Warning: the following pointers check is conceptually wrong - we
      // keep it because the props maintain SMART POINTER-like reference to
      // the associated style. This prevents the style from being destroyed
      // while still referenced by the prop.
      if (!prop || style.getPointer() != prop->getColorStyle()) {
        if (prop) prop->getMutex()->unlock();

        stroke->setProp(style->makeStrokeProp(stroke));
        prop = stroke->getProp();
        if (prop) prop->getMutex()->lock();
      }
    }

    //--------- draw ------------
    if (!prop) return;

    if (pushAttribs) myGlPushAttrib(GL_ALL_ATTRIB_BITS), pushedAttribs = true;

    bool alphaChannel = rd.m_alphaChannel, antialias = rd.m_antiAliasing;
    //TODO
    /*
    TVectorImagePatternStrokeProp *aux =
      dynamic_cast<TVectorImagePatternStrokeProp *>(prop);
    if (aux)  // gli image pattern vettoriali tornano in questa funzione....non
              // facendo il corpo dell'else'si evita di disegnarli due volte!
      prop->draw(rd);
    else */
    {
      //TODO
      /*
      if (antialias)
        tglEnableLineSmooth(true);
      else
        tglEnableLineSmooth(false);
      */

      glEnable(GL_BLEND);

      TPixel32 color = prop->getColorStyle()->getMainColor();
      if (color.m == 0) return;
      // obtain outline points
      const std::vector<TOutlinePoint>& v = prop->getOutlinePointArray(rd);
      if (v.empty()) return;

      ///std::cout << "-----------" << std::endl;
      ///std::cout << "projectionMatrix" << std::endl;
      ///printMatrix(m_projectionMatrix);
      ///std::cout << "VPMatrix" << std::endl;
      ///printMatrix(m_MVPMatrix);
      ///std::cout << "toQMatrix(rd.m_aff)" << std::endl;
      ///printMatrix(toQMatrix(rd.m_aff));
      QMatrix4x4 MVPMatrix = m_MVPMatrix * m_modelMatrix * toQMatrix(rd.m_aff);
      ///std::cout << "MVPMatrix" << std::endl;
      ///printMatrix(MVPMatrix);

      m_smoothShader.program->bind();
      m_smoothShader.program->setUniformValue(m_smoothShader.mvpMatrixUniform, MVPMatrix);
      m_smoothShader.program->enableAttributeArray(m_smoothShader.vertexAttrib);
      m_smoothShader.program->setUniformValue(m_smoothShader.colorUniform, toQColor(color));

      static const int stride = sizeof(TOutlinePoint);
      m_smoothShader.program->setAttributeArray(m_smoothShader.vertexAttrib, GL_DOUBLE, &v[0], 2, stride);

      if (alphaChannel) {
        GLboolean channels[4];
        glGetBooleanv(GL_COLOR_WRITEMASK, &channels[0]);

        // Draw RGB channels
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColorMask(channels[0], channels[1], channels[2], GL_FALSE);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, v.size());

        // Draw Matte channel
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, channels[3]);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, v.size());

        glColorMask(channels[0], channels[1], channels[2], channels[3]);
      }
      else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, v.size());
      }
      m_smoothShader.program->disableAttributeArray(m_smoothShader.vertexAttrib);

    }

    if (pushAttribs) myGlPopAttrib(), pushedAttribs = false;

    prop->getMutex()->unlock();
    //---------------------
  }
  catch (...) {
    if (prop) prop->getMutex()->unlock();
    if (pushedAttribs) myGlPopAttrib();
  }
  assert((glGetError()) == GL_NO_ERROR);

}
//-----------------------------------------------------------------------------

void OpenGLViewerDraw::drawVector(const TVectorRenderData &rd, TRegion *r,
  bool pushAttribs) {

}
//-----------------------------------------------------------------------------
