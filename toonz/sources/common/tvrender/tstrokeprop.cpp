

//#include "tcolorstyles.h"
#include "tsimplecolorstyles.h"
//#include "tstrokeoutline.h"
#include "tstrokeprop.h"
#include "tgl.h"
//#include "tcolorfunctions.h"
#include "tvectorrenderdata.h"
#include "tmathutil.h"
//#include "tcurves.h"
//#include "tstrokeutil.h"

//#include "tstroke.h"
//#include "tflash.h"

//=============================================================================

TSimpleStrokeProp::TSimpleStrokeProp(const TStroke *stroke,
                                     TSimpleStrokeStyle *style)
    : TStrokeProp(stroke)
    , m_colorStyle(style)

{
  m_styleVersionNumber = m_colorStyle->getVersionNumber();
  m_colorStyle->addRef();
}

//-----------------------------------------------------------------------------

TSimpleStrokeProp::~TSimpleStrokeProp() { m_colorStyle->release(); }

//-----------------------------------------------------------------------------

const TColorStyle *TSimpleStrokeProp::getColorStyle() const {
  return m_colorStyle;
}

//-----------------------------------------------------------------------------

TStrokeProp *TSimpleStrokeProp::clone(const TStroke *stroke) const {
  TSimpleStrokeProp *prop = new TSimpleStrokeProp(stroke, m_colorStyle);
  prop->m_strokeChanged   = m_strokeChanged;
  return prop;
}

//-----------------------------------------------------------------------------

void TSimpleStrokeProp::draw(
    const TVectorRenderData
        &rd) /*assenza di const non e' una dimenticanza! Alcune sottoclassi
                devono ridefinire questo metodo e serve che non sia const*/
{
  if (rd.m_clippingRect != TRect() && !rd.m_is3dView &&
      !convert(rd.m_aff * m_stroke->getBBox()).overlaps(rd.m_clippingRect))
    return;

  if (!rd.m_show0ThickStrokes) {
    // >:(  This is not an implementation detail of TCenterlineStrokeStyle
    // because the drawStroke()
    //      function does not have access to rd - should modify the interface...
    //      it would be best.

    const TCenterLineStrokeStyle *cs =
        dynamic_cast<const TCenterLineStrokeStyle *>(m_colorStyle);
    if (cs && cs->getParamValue(TColorStyle::double_tag(), 0) == 0) return;
  }

  glPushMatrix();
  tglMultMatrix(rd.m_aff);
  m_colorStyle->drawStroke(rd.m_cf, m_stroke);

  glPopMatrix();
}

//-----------------------------------------------------------------------------

void TSimpleStrokeProp::draw(TFlash &flash) {
  getColorStyle()->drawStroke(flash, getStroke());
}
//=============================================================================

TRasterImagePatternStrokeProp::TRasterImagePatternStrokeProp(
    const TStroke *stroke, TRasterImagePatternStrokeStyle *style)
    : TStrokeProp(stroke), m_colorStyle(style) {
  m_styleVersionNumber = style->getVersionNumber();
  m_colorStyle->addRef();
}

//-----------------------------------------------------------------------------

TRasterImagePatternStrokeProp::~TRasterImagePatternStrokeProp() {
  m_colorStyle->release();
}

//-----------------------------------------------------------------------------

const TColorStyle *TRasterImagePatternStrokeProp::getColorStyle() const {
  return m_colorStyle;
}

//-----------------------------------------------------------------------------

TStrokeProp *TRasterImagePatternStrokeProp::clone(const TStroke *stroke) const {
  TRasterImagePatternStrokeProp *prop =
      new TRasterImagePatternStrokeProp(stroke, m_colorStyle);
  prop->m_strokeChanged   = m_strokeChanged;
  prop->m_transformations = m_transformations;
  return prop;
}

//-----------------------------------------------------------------------------

void TRasterImagePatternStrokeProp::draw(
    const TVectorRenderData &rd) /*assenza di const non e' una
                                    dimenticanza! Alcune
                                    sottoclassi devono
                                    ridefinire questo metodo e
                                    serbve che non sia const*/
{
  if (rd.m_clippingRect != TRect() && !rd.m_is3dView &&
      !convert(rd.m_aff * m_stroke->getBBox()).overlaps(rd.m_clippingRect))
    return;

  if (m_strokeChanged ||
      m_styleVersionNumber != m_colorStyle->getVersionNumber()) {
    m_strokeChanged      = false;
    m_styleVersionNumber = m_colorStyle->getVersionNumber();
    m_colorStyle->computeTransformations(m_transformations, m_stroke);
  }
  m_colorStyle->drawStroke(rd, m_transformations, m_stroke);
}

//-----------------------------------------------------------------------------

void TRasterImagePatternStrokeProp::draw(TFlash &flash) {
  getColorStyle()->drawStroke(flash, getStroke());
}

//-----------------------------------------------------------------------------
//=============================================================================

TVectorImagePatternStrokeProp::TVectorImagePatternStrokeProp(
    const TStroke *stroke, TVectorImagePatternStrokeStyle *style)
    : TStrokeProp(stroke), m_colorStyle(style) {
  m_styleVersionNumber = style->getVersionNumber();
  m_colorStyle->addRef();
}

//-----------------------------------------------------------------------------

TVectorImagePatternStrokeProp::~TVectorImagePatternStrokeProp() {
  m_colorStyle->release();
}

//-----------------------------------------------------------------------------

const TColorStyle *TVectorImagePatternStrokeProp::getColorStyle() const {
  return m_colorStyle;
}

//-----------------------------------------------------------------------------

TStrokeProp *TVectorImagePatternStrokeProp::clone(const TStroke *stroke) const {
  TVectorImagePatternStrokeProp *prop =
      new TVectorImagePatternStrokeProp(stroke, m_colorStyle);
  prop->m_strokeChanged   = m_strokeChanged;
  prop->m_transformations = m_transformations;
  return prop;
}

//-----------------------------------------------------------------------------

void TVectorImagePatternStrokeProp::draw(
    const TVectorRenderData &rd) /*assenza di const non e' una
                                    dimenticanza! Alcune
                                    sottoclassi devono
                                    ridefinire questo metodo e
                                    serbve che non sia const*/
{
  if (rd.m_clippingRect != TRect() && !rd.m_is3dView &&
      !convert(rd.m_aff * m_stroke->getBBox()).overlaps(rd.m_clippingRect))
    return;

  if (m_strokeChanged ||
      m_styleVersionNumber != m_colorStyle->getVersionNumber()) {
    m_strokeChanged      = false;
    m_styleVersionNumber = m_colorStyle->getVersionNumber();
    m_colorStyle->computeTransformations(m_transformations, m_stroke);
  }
  m_colorStyle->drawStroke(rd, m_transformations, m_stroke);
}

//-----------------------------------------------------------------------------

void TVectorImagePatternStrokeProp::draw(TFlash &flash) {
  getColorStyle()->drawStroke(flash, getStroke());
}

//-----------------------------------------------------------------------------

/*
void TSimpleStrokeProp::draw(TFlash &flash)
{
  // fintissima!!!! quella vera deve risalire per m_colorStyle->drawStroke()
  // come la sua sorellina di sopra
  int i, n = m_stroke->getControlPointCount();
  flash.setColor(m_colorStyle->getMainColor());
  for(i=0;i<n-1;i++)
    {
     TPointD a = m_stroke->getControlPoint(i);
     TPointD b = m_stroke->getControlPoint(i+1);
     flash.drawLine(a,b);
    }
}
*/

//=============================================================================

OutlineStrokeProp::OutlineStrokeProp(const TStroke *stroke,
                                     const TOutlineStyleP style)
    : TStrokeProp(stroke)
    , m_colorStyle(style)
    , m_outline()
    , m_outlinePixelSize(0) {
  m_styleVersionNumber = m_colorStyle->getVersionNumber();
}

//-----------------------------------------------------------------------------

TStrokeProp *OutlineStrokeProp::clone(const TStroke *stroke) const {
  OutlineStrokeProp *prop  = new OutlineStrokeProp(stroke, m_colorStyle);
  prop->m_strokeChanged    = m_strokeChanged;
  prop->m_outline          = m_outline;
  prop->m_outlinePixelSize = m_outlinePixelSize;
  return prop;
}

//-----------------------------------------------------------------------------

const TColorStyle *OutlineStrokeProp::getColorStyle() const {
  return m_colorStyle.getPointer();
}

//-----------------------------------------------------------------------------

void OutlineStrokeProp::draw(const TVectorRenderData &rd) {
  if (rd.m_clippingRect != TRect() && !rd.m_is3dView &&
      !convert(rd.m_aff * m_stroke->getBBox()).overlaps(rd.m_clippingRect))
    return;

  glPushMatrix();
  tglMultMatrix(rd.m_aff);

  ///std::cout << "OutlineStrokeProp::draw rd.m_aff" << std::endl;
  ///std::cout << rd.m_aff.a11 << ", " << rd.m_aff.a12 << ", " << rd.m_aff.a13 << std::endl;
  ///std::cout << rd.m_aff.a21 << ", " << rd.m_aff.a22 << ", " << rd.m_aff.a23 << std::endl << std::endl;


  double pixelSize = sqrt(tglGetPixelSize2());

#ifdef _DEBUG
  if (m_stroke->isCenterLine() && m_colorStyle->getTagId() != 99)
#else
  if (m_stroke->isCenterLine())
#endif
  {
    TCenterLineStrokeStyle *appStyle =
        new TCenterLineStrokeStyle(m_colorStyle->getAverageColor(), 0, 0);
    appStyle->drawStroke(rd.m_cf, m_stroke);
    delete appStyle;
  } else {
    if (!isAlmostZero(pixelSize - m_outlinePixelSize, 1e-5) ||
        m_strokeChanged ||
        m_styleVersionNumber != m_colorStyle->getVersionNumber()) {
      m_strokeChanged    = false;
      m_outlinePixelSize = pixelSize;
      TOutlineUtil::OutlineParameter param;

      m_outline.getArray().clear();
      m_colorStyle->computeOutline(m_stroke, m_outline, param);

      // TOutlineStyle::StrokeOutlineModifier *modifier =
      // m_colorStyle->getStrokeOutlineModifier();
      // if(modifier)
      //  modifier->modify(m_outline);

      m_styleVersionNumber = m_colorStyle->getVersionNumber();
    }

    m_colorStyle->drawStroke(rd.m_cf, &m_outline, m_stroke);
  }

  glPopMatrix();
}

//-----------------------------------------------------------------------------

std::vector<TOutlinePoint> & OutlineStrokeProp::getOutlinePointArray(const TVectorRenderData &rd) {
  if (rd.m_clippingRect != TRect() && !rd.m_is3dView &&
    !convert(rd.m_aff * m_stroke->getBBox()).overlaps(rd.m_clippingRect))
    return std::vector<TOutlinePoint>();

  double pixelSize = sqrt(tglGetPixelSize2());
  ///std::cout << "OutlineStrokeProp::getOutlinePointArray rd.m_aff" << std::endl;
  ///std::cout << rd.m_aff.a11 << ", " << rd.m_aff.a12 << ", " << rd.m_aff.a13 << std::endl;
  ///std::cout << rd.m_aff.a21 << ", " << rd.m_aff.a22 << ", " << rd.m_aff.a23 << std::endl << std::endl;

  //TODO
  /*
#ifdef _DEBUG
  if (m_stroke->isCenterLine() && m_colorStyle->getTagId() != 99)
#else
  if (m_stroke->isCenterLine())
#endif
  {
    TCenterLineStrokeStyle *appStyle =
      new TCenterLineStrokeStyle(m_colorStyle->getAverageColor(), 0, 0);
    appStyle->drawStroke(rd.m_cf, m_stroke);
    delete appStyle;
  }
  else
  */{
    if (!isAlmostZero(pixelSize - m_outlinePixelSize, 1e-5) ||
      m_strokeChanged ||
      m_styleVersionNumber != m_colorStyle->getVersionNumber()) {
      m_strokeChanged = false;
      m_outlinePixelSize = pixelSize;
      TOutlineUtil::OutlineParameter param;

      m_outline.getArray().clear();
      m_colorStyle->computeOutline(m_stroke, m_outline, param);

      m_styleVersionNumber = m_colorStyle->getVersionNumber();
    }

    //m_colorStyle->drawStroke(rd.m_cf, &m_outline, m_stroke);
  }
  return m_outline.getArray();
}

//=============================================================================

void OutlineStrokeProp::draw(TFlash &flash) {
  m_colorStyle->drawStroke(flash, getStroke());
}

//=============================================================================

/* ora e' virtuale pura
void TStrokeProp::draw(TFlash &flash)
{
getColorStyle()->drawStroke(flash, getStroke());
}

*/

//=============================================================================
