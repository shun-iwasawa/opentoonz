#pragma once

#ifndef BOARDSETTINGS_H
#define BOARDSETTINGS_H

#include "traster.h"
#include "tfilepath.h"

// TnzCore includes
#include "tstream.h"

#include <QList>
#include <QRectF>
#include <QColor>
#include <QPainter>

#undef DVAPI
#undef DVVAR
#ifdef TOONZLIB_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

class ToonzScene;

class DVAPI BoardItem {
public:
  enum Type {
    FreeText = 0,     //���R�e�L�X�g (m_text�ɓ��e)
    ProjectName,      //�v���W�F�N�g��
    SceneName,        //�V�[���t�@�C����
    Duration_Frame,   //�����i�t���[�����j
    Duration_SecFrame,//�����i�b�{�R�}�j
    Duration_HHMMSSFF,//�����iHH:MM:SS:FF�j
    CurrentDate,      //���݂̓��i�N / �� / ���j
    CurrentDateTime,  //���݂̓����i�N / �� / �� / �� / �� / �b�j
    UserName,         //���[�U��
    ScenePath_Aliased,//�V�[���t�@�C���p�X�i�G�C���A�X�t�j
    ScenePath_Full,   //�V�[���t�@�C���p�X�i�t���p�X�j
    MoviePath_Aliased,//���[�r�[�t�@�C���p�X�i�G�C���A�X�t�j
    MoviePath_Full,   //���[�r�[�t�@�C���p�X�i�t���p�X�j
    Image,            //�摜 (m_imgPath�Ƀp�X)
    TypeCount
  };

private:
  QString m_name;
  Type m_type;

  // ��ʑS�̂ɑ΂��道�\�L
  QRectF m_rect;
  
  // �����̍ő�T�C�Y
  int m_maximumFontSize;

  // �����̐F
  QColor m_color;

  // �t�H���g
  QFont m_font;

  QString m_text;
  TFilePath m_imgPath;
  
  QString getContentText(ToonzScene*);

public:
  BoardItem();

  QRectF getRatioRect() { return m_rect; }
  void setRatioRect(QRectF rect) { m_rect = rect; }

  QRectF getItemRect(QSize imgSize); //�摜���Rect��Ԃ�
  void drawItem(QPainter& p, QSize imgSize, int shrink, ToonzScene* scene);

  QString getName() { return m_name; }
  void setName(QString name) { m_name = name; }

  Type getType() { return m_type; }
  void setType(Type type) { m_type = type; }

  int getMaximumFontSize() { return m_maximumFontSize; }
  void setMaximumFontSize(int size) { m_maximumFontSize = size; }

  QColor getColor() { return m_color; }
  void setColor(QColor color) { m_color = color; }

  QFont& font() { return m_font; }

  QString getFreeText() { return m_text; }
  void setFreeText(QString text) { m_text = text; }

  TFilePath getImgPath() { return m_imgPath; }
  void setImgPath(TFilePath path) { m_imgPath = path; }

  void saveData(TOStream &os);
  void loadData(TIStream &is);
};

class DVAPI BoardSettings {
  //�L������
  bool m_active = false;
  //�t���[����
  int m_duration = 0;
  //�e�p�[�c
  QList<BoardItem> m_items;

public:
  BoardSettings();

  QImage getBoardImage(TDimension& dim, int shrink, ToonzScene* scene);

  TRaster32P getBoardRaster(TDimension& dim, int shrink, ToonzScene* scene);

  int getDuration() { return m_duration; }

  bool isActive() { return m_active; }
  void setActive(bool on) { m_active = on; }

  int getItemCount() { return m_items.count(); }
  BoardItem& getItem(int index) { return m_items[index]; }

  void setDuration(int f) { m_duration = f; }
  
  void addNewItem(int insertAt = 0);
  void removeItem(int index);

  void saveData(TOStream &os, bool forPreset = false);
  void loadData(TIStream &is);
};

#endif