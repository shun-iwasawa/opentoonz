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
    //��������I�I
  };

private:

  // ��ʑS�̂ɑ΂��道�\�L
  QRectF m_rect;
  
  // �����̍ő�T�C�Y
  int m_maximumFontSize;

  // �����̐F
  QColor m_color;

  // �t�H���g

public:
  BoardItem(){}
};

class DVAPI BoardSettings {
  //�L������
  bool m_active;
  //�t���[����
  int m_duration;
  //�w�i�摜
  TFilePath m_bgPath;
  //�e�p�[�c
  QList<BoardItem> m_items;

public:
  BoardSettings();

  TRaster32P getBoardRaster(TDimension& dim, int shrink);

};

#endif