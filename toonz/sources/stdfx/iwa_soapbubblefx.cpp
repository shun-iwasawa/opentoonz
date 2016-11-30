/*------------------------------------
Iwa_SoapBubbleFx
Generates thin film inteference colors from two reference images;
one is for a thickness and the other one is for a normal vector
of the film.
Inherits Iwa_SpectrumFx.
------------------------------------*/

#include "iwa_soapbubblefx.h"
#include "iwa_cie_d65.h"
#include "iwa_xyz.h"

#include <QList>
#include <QPoint>

namespace {
  const float PI = 3.14159265f;



}

//------------------------------------

Iwa_SoapBubbleFx::Iwa_SoapBubbleFx()
: Iwa_SpectrumFx()
, m_binarize_threshold(0.5)
, m_normal_sample_distance(1) {
  removeInputPort("Source");
  removeInputPort("Light"); /* not used */
  addInputPort("Thickness", m_input);
  addInputPort("Shape", m_shape);
  addInputPort("Depth", m_depth);

  bindParam(this, "normalSampleDistance", m_normal_sample_distance);
  m_binarize_threshold->setValueRange(0.01, 0.99);
  m_normal_sample_distance->setValueRange(1, 10);
}

//------------------------------------

void Iwa_SoapBubbleFx::doCompute(TTile &tile, double frame,
  const TRenderSettings &settings){
  if (!m_input.isConnected()) return;
  if (!m_shape.isConnected() && !m_depth.isConnected()) return;

  TDimensionI dim(tile.getRaster()->getLx(), tile.getRaster()->getLy());
  TRectD bBox(tile.m_pos, TPointD(dim.lx,dim.ly));

  /* soap bubble color map */
  TRasterGR8P bubbleColor_ras(sizeof(float3) * 256 * 256, 1);
  bubbleColor_ras->lock();
  float3* bubbleColor_p = (float3 *)bubbleColor_ras->getRawData();
  calcBubbleMap(bubbleColor_p, frame, true);

  /* depth map */
  //int sampleDistance = m_normal_sample_distance->getValue();
  //TDimensionI enlargedDim = dim + TDimensionI(sampleDistance * 2, sampleDistance * 2);
  //TRectD enlargedBBox = bBox.enlarge((double)sampleDistance);
  TRasterGR8P depth_map_ras(sizeof(float) * dim.lx * dim.ly, 1);
  depth_map_ras->lock();
  float* depth_map_p = (float *)depth_map_ras->getRawData();

  /* if the depth image is connected, use it */
  if (m_depth.isConnected()){
    TTile depth_tile;
    m_depth->allocateAndCompute(depth_tile, bBox.getP00(), dim, tile.getRaster(), frame, settings);
    TRasterP depthRas = depth_tile.getRaster();
    depthRas->lock();

    TRaster32P depthRas32 = (TRaster32P)depthRas;
    TRaster64P depthRas64 = (TRaster64P)depthRas;
    {
      if (depthRas32)
        convertToBrightness<TRaster32P, TPixel32>(depthRas32, depth_map_p, dim);
      else if (depthRas64)
        convertToBrightness<TRaster64P, TPixel64>(depthRas64, depth_map_p, dim);
    }
    depthRas->unlock();
  }
  /* or, use the shape image to obtain pseudo depth */
  else { // m_shape.isConnected
    // obtain shape image
    TTile shape_tile;
    {
      TRaster32P tmp(1,1);
      m_shape->allocateAndCompute(shape_tile, bBox.getP00(), dim, tmp, frame, settings);
    }
    processShape(frame, shape_tile, depth_map_p, dim);
  }

  // compute the thickness input and temporary store to the tile 
  m_input->compute(tile, frame, settings);

  TRaster32P ras32 = (TRaster32P)tile.getRaster();
  TRaster64P ras64 = (TRaster64P)tile.getRaster();
  if (ras32)
    convertToRaster<TRaster32P, TPixel32>(ras32, depth_map_p, dim, bubbleColor_p);
  else if (ras64)
    convertToRaster<TRaster64P, TPixel64>(ras64, depth_map_p, dim, bubbleColor_p);


  depth_map_ras->unlock();
  bubbleColor_ras->unlock();
}

//------------------------------------

template <typename RASTER, typename PIXEL>
void Iwa_SoapBubbleFx::convertToBrightness(const RASTER srcRas, float* dst, TDimensionI dim) {
  float* dst_p = dst;
  for (int j = 0; j < dim.ly; j++) {
    PIXEL *pix = srcRas->pixels(j);
    for (int i = 0; i < dim.lx; i++, dst_p++, pix++) {
      float r = (float)pix->r / (float)PIXEL::maxChannelValue;
      float g = (float)pix->g / (float)PIXEL::maxChannelValue;
      float b = (float)pix->b / (float)PIXEL::maxChannelValue;
      /* brightness */
      *dst_p = 0.298912f * r + 0.586611f * g + 0.114478f * b;
    }
  }
}

//------------------------------------

template <typename RASTER, typename PIXEL>
void Iwa_SoapBubbleFx::convertToRaster(const RASTER ras, float* depth_map_p, TDimensionI dim, float3* bubbleColor_p) {
  float* depth_p = depth_map_p;
  for (int j = 0; j < dim.ly; j++) {
    PIXEL *pix = ras->pixels(j);
    for (int i = 0; i < dim.lx; i++, depth_p++, pix++) {
      float alpha = (float)pix->m / PIXEL::maxChannelValue;
      if (alpha == 0.0f) /* no change for the transparent pixels */
        continue;

      /* Not doing unpremultiply - since it causes a large margin of conversion error */
      float red   = (float)pix->r / (float)PIXEL::maxChannelValue;
      float green = (float)pix->g / (float)PIXEL::maxChannelValue;
      float blue  = (float)pix->b / (float)PIXEL::maxChannelValue;
      float brightness = 0.298912f * red + 0.586611f * green + 0.114478f * blue;

      float coordinate[2];
      coordinate[0] = 256.0f * std::min(1.0f, *depth_p);
      coordinate[1] = 256.0f * brightness;
      
      int neighbors[2][2];
      
      // interpolate sampling
      if (coordinate[0] <= 0.5f)
        neighbors[0][0] = 0;
      else
        neighbors[0][0] = (int)std::floorf(coordinate[0] - 0.5f);
      if (coordinate[0] >= 255.5f)
        neighbors[0][1] = 255;
      else
        neighbors[0][1] = (int)std::floorf(coordinate[0] + 0.5f);
      if (coordinate[1] <= 0.5f)
        neighbors[1][0] = 0;
      else
        neighbors[1][0] = (int)std::floorf(coordinate[1] - 0.5f);
      if (coordinate[1] >= 255.5f)
        neighbors[1][1] = 255;
      else
        neighbors[1][1] = (int)std::floorf(coordinate[1] + 0.5f);

      float interp_ratio[2];
      interp_ratio[0] = coordinate[0] - 0.5f - std::floorf(coordinate[0] - 0.5f);
      interp_ratio[1] = coordinate[1] - 0.5f - std::floorf(coordinate[1] - 0.5f);

      red = bubbleColor_p[neighbors[0][0] * 256 + neighbors[1][0]].x * (1.0f - interp_ratio[0]) * (1.0f - interp_ratio[1])
        + bubbleColor_p[neighbors[0][1] * 256 + neighbors[1][0]].x * interp_ratio[0] * (1.0f - interp_ratio[1])
        + bubbleColor_p[neighbors[0][0] * 256 + neighbors[1][1]].x * (1.0f - interp_ratio[0]) * interp_ratio[1]
        + bubbleColor_p[neighbors[0][1] * 256 + neighbors[1][1]].x * interp_ratio[0] * interp_ratio[1];
      green = bubbleColor_p[neighbors[0][0] * 256 + neighbors[1][0]].y * (1.0f - interp_ratio[0]) * (1.0f - interp_ratio[1])
        + bubbleColor_p[neighbors[0][1] * 256 + neighbors[1][0]].y * interp_ratio[0] * (1.0f - interp_ratio[1])
        + bubbleColor_p[neighbors[0][0] * 256 + neighbors[1][1]].y * (1.0f - interp_ratio[0]) * interp_ratio[1]
        + bubbleColor_p[neighbors[0][1] * 256 + neighbors[1][1]].y * interp_ratio[0] * interp_ratio[1];
      blue = bubbleColor_p[neighbors[0][0] * 256 + neighbors[1][0]].z * (1.0f - interp_ratio[0]) * (1.0f - interp_ratio[1])
        + bubbleColor_p[neighbors[0][1] * 256 + neighbors[1][0]].z * interp_ratio[0] * (1.0f - interp_ratio[1])
        + bubbleColor_p[neighbors[0][0] * 256 + neighbors[1][1]].z * (1.0f - interp_ratio[0]) * interp_ratio[1]
        + bubbleColor_p[neighbors[0][1] * 256 + neighbors[1][1]].z * interp_ratio[0] * interp_ratio[1];

      /* clamp */
      float val = red * (float)PIXEL::maxChannelValue + 0.5f;
      pix->r = (typename PIXEL::Channel)((val > (float)PIXEL::maxChannelValue)
        ? (float)PIXEL::maxChannelValue : val);
      val = green * (float)PIXEL::maxChannelValue + 0.5f;
      pix->g = (typename PIXEL::Channel)((val > (float)PIXEL::maxChannelValue)
        ? (float)PIXEL::maxChannelValue : val);
      val = blue * (float)PIXEL::maxChannelValue + 0.5f;
      pix->b = (typename PIXEL::Channel)((val > (float)PIXEL::maxChannelValue)
        ? (float)PIXEL::maxChannelValue : val);
    }
  }

}

//------------------------------------

void Iwa_SoapBubbleFx::processShape(double frame, TTile& shape_tile, float* depth_map_p, TDimensionI dim) {

  TRaster32P shapeRas = shape_tile.getRaster();
  shapeRas->lock();

  // binarize the shape image
  TRasterGR8P binarized_ras(sizeof(char) * dim.lx * dim.ly, 1);
  binarized_ras->lock();
  char* binarized_p = (char*)binarized_ras->getRawData();
  float binarize_thres = (float)m_binarize_threshold->getValue(frame);

  do_binarize(shapeRas, binarized_p, binarize_thres, dim);

  shapeRas->unlock();


//Input: A square tessellation, T, containing a connected component P of black cells.
//
//Output: A sequence B (b1, b2 ,..., bk) of boundary pixels i.e. the contour.
//
//Define M(a) to be the Moore neighborhood of pixel a. 
//Let p denote the current boundary pixel. 
//Let c denote the current pixel under consideration i.e. c is in M(p).
//
//Begin
//
//Set B to be empty.
//From bottom to top and left to right scan the cells of T until a black pixel, s, of P is found.

  
  //Insert s in B.
//Set the current boundary point p to s i.e. p=s
//Backtrack i.e. move to the pixel from which s was entered.
//Set c to be the next clockwise pixel in M(p).
//While c not equal to s do
//
//   If c is black
//insert c in B
//set p=c
//backtrack (move the current pixel c to the pixel from which p was entered)
//   else
//advance the current pixel c to the next clockwise pixel in M(p)
//end While
//End 

  // trace edges
  QList<QList<QPoint>> edgePoints;
  //端からスキャンラインで走査する
  char* pix = binarized_p;
  for (int j = 0; j < dim.ly; j++){
    char pre_pix = 0;
    for (int i = 0; i < dim.lx; i++, pix++){
      // まだコードが振られておらず、前のピクセルが0で、このピクセルが1の場合
      // ループを開始する
      if ((*pix) == 1 && pre_pix == 0){
        QList<QPoint> pointSet;
        QPoint curPos(i, j);
        pointSet.append(curPos);
        
      }

      // 前のピクセルの値を格納
      pre_pix = *pix;
    }
  }
  
  
  
  //do_thinning(binarized_p, dim);

  binarized_ras->unlock();


  // create (gained) blur filter

  // blur filtering

}

//------------------------------------

void Iwa_SoapBubbleFx::do_binarize(TRaster32P srcRas, char* dst_p, float thres, TDimensionI dim){
  TPixel32::Channel channelThres = (TPixel32::Channel)(thres*(float)TPixel32::maxChannelValue);
  char* tmp_p = dst_p;
  for (int j = 0; j < dim.ly; j++) {
    TPixel32* pix = srcRas->pixels(j);
    for (int i = 0; i < dim.lx; i++, pix++, tmp_p){
      (*tmp_p) = (pix->m > channelThres) ? 1 : 0;
    }
  }
}

//------------------------------------
#if 0
void Iwa_SoapBubbleFx::do_thinning(char* target_p, TDimensionI dim){
  // thinning
  while (1){
    char* tmp_p = target_p;
    // scanning inside of the source image
    for (int j = 1; j < dim.ly - 1; j++){
      for (int i = 1; i < dim.lx - 1; i++, tmp_p++){
        if (*tmp_p == 1)
          *tmp_p = thin_line_judge(i, j, dim, target_p);
      }
    }
    //背景にできる画素をfalseにする
    count = 0;
    for (y = 1; y<param_ptr->line_tex.dims.y - 1; y++){
      for (x = 1; x<param_ptr->line_tex.dims.x - 1; x++){
        if (*thin_data(x, y, 8, 0) == -1){
          *thin_data(x, y, 8, 0) = 0;
          count++;
          param_ptr->binary.pixel_amount--;//細線化後の総ピクセル数になるといいね

        }
      }
    }

    if (count == 0)
      break;
  }


  //------------------------
  //　チェインコードにする
  //------------------------

  static char* data_ptr;
  static int tmp;
  data_ptr = param_ptr->thin_line.data;
  tmp = 0;
  delete param_ptr->thin_line.chain_code;
  param_ptr->thin_line.chain_code = new char[param_ptr->binary.pixel_amount];
  static int current_pos[2];
  static int entry_cc;
  static int current_cc;

  static int i;
  static int cc;

  static int chain_pos_x[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
  static int chain_pos_y[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };

  while (1){
    if (*data_ptr != 0){
      param_ptr->thin_line.start_pos[0] = tmp % param_ptr->line_tex.dims.x;
      param_ptr->thin_line.start_pos[1] = tmp / param_ptr->line_tex.dims.x;
      break;
    }
    data_ptr++;
    tmp++;
  }

  static unsigned char* thin_line_data_8_p;

  //テクスチャ初期化

  std::memset(param_ptr->thin_line.data_8, 0, param_ptr->line_tex.reso_for_tex*param_ptr->line_tex.reso_for_tex * 3);

  /*
  for(thin_line_data_8_p = &param_ptr->thin_line.data_8[0];
  thin_line_data_8_p <= &param_ptr->thin_line.data_8[param_ptr->line_tex.dims.x*param_ptr->line_tex.dims.y*3-1];
  thin_line_data_8_p++){
  *thin_line_data_8_p = (unsigned char)0;
  }
  */

  current_pos[0] = param_ptr->thin_line.start_pos[0];
  current_pos[1] = param_ptr->thin_line.start_pos[1];

  //進入点を赤くする
  //param_ptr->thin_line.data_8[(current_pos[1]*param_ptr->line_tex.dims.x+current_pos[0])*3] = (unsigned char)255;

  //バウンディングボックス初期化
  param_ptr->thin_line.xmax = current_pos[0];
  param_ptr->thin_line.xmin = current_pos[0];
  param_ptr->thin_line.ymax = current_pos[1];
  param_ptr->thin_line.ymin = current_pos[1];

  entry_cc = 4;//左から進入

  //やってみよう…細線の重心を求める
  static long int pos_sum[2];
  pos_sum[0] = 0;
  pos_sum[1] = 1;

  //std::cout<<"pixel_amount = "<<param_ptr->binary.pixel_amount<<"\n";

  //------チェインコード格納ループ。連結の数だけ繰り返す
  for (i = 0; i<param_ptr->binary.pixel_amount; i++){

    //細線の重心を求めるため、位置を足しこむ
    pos_sum[0] += current_pos[0];
    pos_sum[1] += current_pos[1];

    //８近傍をぐるっと見る
    for (cc = 1; cc <= 7; cc++){
      current_cc = (entry_cc + cc) % 8;
      if (*thin_data(current_pos[0], current_pos[1], current_cc, 0) == 1){//次の行き先発見！
        break;
      }
    }

    param_ptr->thin_line.chain_code[i] = current_cc;//チェインコードを格納

    //連結ぶりをチェインコードに記録.
    //+0:濃さ1 +8:濃さ1.118..　+16:濃さ1.4142..
    if (i != 0){//先端は一番最後に格納
      if (current_cc % 2 == 0 && param_ptr->thin_line.chain_code[i - 1] % 2 == 0)
      {/*何もしない*/
      }
      else if (current_cc % 2 + param_ptr->thin_line.chain_code[i - 1] % 2 == 1)
        param_ptr->thin_line.chain_code[i] += 8;
      else
        param_ptr->thin_line.chain_code[i] += 16;
    }

    if (param_ptr->thin_line.chain_code[i] / 8 == 0){
      for (c = 0; c<3; c++)
        param_ptr->thin_line.data_8[(current_pos[1] * param_ptr->line_tex.reso_for_tex + current_pos[0]) * 3 + c] = (unsigned char)255;
      //param_ptr->thin_line.data_8[(current_pos[1]*param_ptr->line_tex.dims.x+current_pos[0])*3+c] = (unsigned char)255;
    }
    else if (param_ptr->thin_line.chain_code[i] / 8 == 1)
      param_ptr->thin_line.data_8[(current_pos[1] * param_ptr->line_tex.reso_for_tex + current_pos[0]) * 3 + 1] = (unsigned char)255;
    //param_ptr->thin_line.data_8[(current_pos[1]*param_ptr->line_tex.dims.x+current_pos[0])*3+1] = (unsigned char)255;
    else if (param_ptr->thin_line.chain_code[i] / 8 == 2)
      param_ptr->thin_line.data_8[(current_pos[1] * param_ptr->line_tex.reso_for_tex + current_pos[0]) * 3 + 2] = (unsigned char)255;
    //param_ptr->thin_line.data_8[(current_pos[1]*param_ptr->line_tex.dims.x+current_pos[0])*3+2] = (unsigned char)255;

    current_pos[0] += chain_pos_x[current_cc];
    current_pos[1] += chain_pos_y[current_cc];//次の位置にカレントを移動

    //バウンディングボックスの更新
    if (param_ptr->thin_line.xmax < current_pos[0])
      param_ptr->thin_line.xmax = current_pos[0];
    if (param_ptr->thin_line.xmin > current_pos[0])
      param_ptr->thin_line.xmin = current_pos[0];
    if (param_ptr->thin_line.ymax < current_pos[1])
      param_ptr->thin_line.ymax = current_pos[1];
    if (param_ptr->thin_line.ymin > current_pos[1])
      param_ptr->thin_line.ymin = current_pos[1];

    entry_cc = (current_cc + 4) % 8;//次のエントリポイント


    //std::cout<<(int)param_ptr->thin_line.chain_code[i];
    //if(current_pos[0] == param_ptr->thin_line.start_pos[0] 
    //	&&current_pos[1] == param_ptr->thin_line.start_pos[1])
    //		std::cout<<"一周したYO！\n";

  }

  if (param_ptr->thin_line.chain_code[0] % 2 == 0 && param_ptr->thin_line.chain_code[param_ptr->binary.pixel_amount - 1] % 2 == 0)
  {/*何もしない*/
  }
  else if (param_ptr->thin_line.chain_code[0] % 2 + param_ptr->thin_line.chain_code[param_ptr->binary.pixel_amount - 1] % 2 == 1)
    param_ptr->thin_line.chain_code[0] += 8;
  else
    param_ptr->thin_line.chain_code[0] += 16;

  //上下に10pixelsのマージンをつける
  param_ptr->thin_line.xmin = max(0, param_ptr->thin_line.xmin - 10);
  param_ptr->thin_line.ymin = max(0, param_ptr->thin_line.ymin - 10);
  param_ptr->thin_line.xmax = min(param_ptr->line_tex.dims.x - 1, param_ptr->thin_line.xmax + 10);
  param_ptr->thin_line.ymax = min(param_ptr->line_tex.dims.y - 1, param_ptr->thin_line.ymax + 10);

  //細線の重心を求める
  param_ptr->thin_line.center_pos[0] = (float)((double)pos_sum[0] / (double)param_ptr->binary.pixel_amount);
  param_ptr->thin_line.center_pos[1] = (float)((double)pos_sum[1] / (double)param_ptr->binary.pixel_amount);
}

//------------------------------------

char Iwa_SoapBubbleFx::thin_line_judge(int x, int y, TDimensionI dim, const char* target_p) {

  struct Locals{
    TDimensionI _dim;
    const char* _target_p;
    char data(int px, int py){
      if (px < 0 || _dim.lx <= px || py < 0 || _dim.ly <= py) return 0;
      return _target_p[py*_dim.lx + px];
    }
  }locals = {dim, target_p};

  // - - - - - - - - -

  int dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
  int dy[8] = { 0, -1, -1, -1, 0, 1, 1, 1 };
  //static int sum, sum2, c1, c2, c3, n, i;
  //static char sub[3][3] = { { 0 } };
  
  // condition #2 - boundary pixel
  int sum = 0;
  for (int n = 0; n < 8; n += 2){//4 neighbors
    sum += !(locals.data(x + dx[n], y + dy[n]));
  }
  if (!(sum >= 1)){
    return(*thin_data(x, y, 8, id));
  }

  // 条件3 - 端点を削除しない条件
  sum = 0;
  for (n = 0; n < 8; n++){
    sum += abs(*thin_data(x + px[n], y + py[n], 8, id));
  }
  if (!(sum >= 2)){
    return(*thin_data(x, y, 8, id));
  }

  // 条件4 - 孤立点を保存する条件
  sum = 0;
  for (n = 0; n < 8; n++){
    if (*thin_data(x + px[n], y + py[n], 8, id) == 1){
      sum++;
    }
  }
  if (!(sum >= 1)){
    return(*thin_data(x, y, 8, id));
  }

  // 条件5 - 連結性を保存する条件
  sum = 0;
  for (n = 0; n < 8; n += 2){
    c1 = (abs(*thin_data(x + px[n], y + py[n], 8, id)) == 1) ? 1 : 0;
    c2 = (abs(*thin_data(x + px[n + 1], y + py[n + 1], 8, id)) == 1) ? 1 : 0;
    c3 = (abs(*thin_data(x + px[(n + 2) % 8], y + py[(n + 2) % 8], 8, id)) == 1) ? 1 : 0;
    sum += !(c1)-!(c1)* !(c2)* !(c3);
  }
  if (!(sum == 1)){
    return(*thin_data(x, y, 8, id));
  }

  // 条件6 - 線幅が2の線分に対して、その片側のみを削除する条件
  sum2 = 0;
  for (i = 0; i < 8; i++){
    if (*thin_data(x + px[i], y + py[i], 8, id) != -1){
      sum2++;
      continue;
    }
    // subに対象画素の8近傍を格納
    for (n = 0; n < 8; n++){
      sub[1 + py[n]][1 + px[n]] = *thin_data(x + px[n], y + py[n], 8, id);
    }
    sub[1 + py[i]][1 + px[i]] = 0;

    sum = 0;
    for (int n = 0; n < 8; n += 2){
      c1 = (abs(sub[1 + py[n]][1 + px[n]]) == 1) ? 1 : 0;
      c2 = (abs(sub[1 + py[n + 1]][1 + px[n + 1]]) == 1) ? 1 : 0;
      c3 = (abs(sub[1 + py[(n + 2) % 8]][1 + px[(n + 2) % 8]]) == 1) ? 1 : 0;
      sum += !(c1)-!(c1)* !(c2)* !(c3);
    }
    if (!(sum == 1)){
      break;
    }
    sum2++;
  }
  return((sum2 == 8) ? -1 : *thin_data(x, y, 8, id));
}
#endif 

//==============================================================================

FX_PLUGIN_IDENTIFIER(Iwa_SoapBubbleFx, "iwa_SoapBubbleFx");