#pragma once

/*------------------------------------
Iwa_SoapBubbleFx
Generates thin film inteference colors from two reference images;
one is for a thickness and the other one is for a normal vector
of the film.
Inherits Iwa_SpectrumFx.
------------------------------------*/

#ifndef IWA_SOAPBUBBLE_H
#define IWA_SOAPBUBBLE_H

#include "iwa_spectrumfx.h"
//#include "stdfx.h"
//#include "tfxparam.h"

class Iwa_SoapBubbleFx final : public Iwa_SpectrumFx {
  FX_PLUGIN_DECLARATION(Iwa_SoapBubbleFx)

protected:
  /* target shape, used to create a pseudo normal vector */
  TRasterFxPort m_shape;
  /* another option, to input a depth map directly */
  TRasterFxPort m_depth;

  TDoubleParamP m_binarize_threshold;
  TIntParamP m_normal_sample_distance;
  
  template <typename RASTER, typename PIXEL>
  void convertToBrightness(const RASTER srcRas, float* dst, TDimensionI dim);

  template <typename RASTER, typename PIXEL>
  void convertToRaster(const RASTER ras, float* depth_map_p, TDimensionI dim, float3* bubbleColor_p);

  void processShape(double frame, TTile& shape_tile, float* depth_map_p, TDimensionI dim);

  void do_binarize(TRaster32P srcRas, char* dst_p, float thres, TDimensionI dim);
  void do_thinning(char* target_p, TDimensionI dim);
  char thin_line_judge(int x, int y, TDimensionI dim, const char* target_p);



public:
  Iwa_SoapBubbleFx();
  
  void doCompute(TTile &tile, double frame,
    const TRenderSettings &settings) override;
};



#endif