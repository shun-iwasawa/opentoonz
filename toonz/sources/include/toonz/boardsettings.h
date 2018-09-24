#pragma once

#ifndef BOARDSETTINGS_H
#define BOARDSETTINGS_H

#include "traster.h"
#include "tfilepath.h"

#include <QList>
#include <QRectF>
#include <QColor>

#undef DVAPI
#undef DVVAR
#ifdef TOONZLIB_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

class DVAPI BoardItem {
public:
  enum Type {
    //ここから！！
  };

private:

  // 画面全体に対する％表記
  QRectF m_rect;
  
  // 文字の最大サイズ
  int m_maximumFontSize;

  // 文字の色
  QColor m_color;

  // フォント

public:
  BoardItem(){}
};

class DVAPI BoardSettings {
  //有効無効
  bool m_active;
  //フレーム長
  int m_duration;
  //背景画像
  TFilePath m_bgPath;
  //各パーツ
  QList<BoardItem> m_items;

public:
  BoardSettings();

  TRaster32P getBoardRaster(TDimension& dim, int shrink);

};

#endif