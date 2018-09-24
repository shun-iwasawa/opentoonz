#include "toonz/boardsettings.h"

BoardSettings::BoardSettings() {

}

TRaster32P BoardSettings::getBoardRaster(TDimension& dim, int shrink) {
  TRaster32P boardRas(dim);
  boardRas->fill(TPixel32::Green);
  return boardRas;
}
