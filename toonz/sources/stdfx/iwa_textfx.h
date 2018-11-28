#pragma once

#ifndef IWA_TEXTFX_H
#define IWA_TEXTFX_H

#include "stdfx.h"
#include "tfxparam.h"
#include "tparamset.h"

//******************************************************************
//	Iwa_Text Fx  class
//******************************************************************

class Iwa_TextFx final : public TStandardZeraryFx {
  FX_PLUGIN_DECLARATION(Iwa_TextFx)

  QString m_noteLevelStr;

protected:
  TIntEnumParamP m_targetType;
  TIntParamP m_columnIndex;
  TStringParamP m_text;

  TIntEnumParamP m_hAlign;

  TPointParamP m_center;
  TDoubleParamP m_width;
  TDoubleParamP m_height;

  TFontParamP m_font;
  TPixelParamP m_textColor;
  TPixelParamP m_boxColor;
  TBoolParamP m_showBorder;

  template <typename RASTER, typename PIXEL>
  void putTextImage(const RASTER srcRas, TPoint &pos, QImage &img);

public:
  enum SourceType { NEARBY_COLUMN, SPECIFIED_COLUMN, INPUT_TEXT };

  Iwa_TextFx();

  bool isZerary() const override { return true; }

  bool canHandle(const TRenderSettings &info, double frame) override {
    return true;
  }

  bool doGetBBox(double frame, TRectD &bBox,
                 const TRenderSettings &ri) override;
  void doCompute(TTile &tile, double frame, const TRenderSettings &ri) override;
  void getParamUIs(TParamUIConcept *&concepts, int &length) override;

  std::string getAlias(double frame,
                       const TRenderSettings &info) const override;

  void setNoteLevelStr(QString str) { m_noteLevelStr = str; }

  SourceType getSourceType() { return (SourceType)(m_targetType->getValue()); }
  int getNoteColumnIndex() { return m_columnIndex->getValue() - 1; }
};
#endif