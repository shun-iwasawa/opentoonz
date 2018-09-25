#pragma once

#ifndef BOARDSETTINGS_H
#define BOARDSETTINGS_H

#include "traster.h"
#include "tfilepath.h"

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
    Image             //�摜 (m_imgPath�Ƀp�X)
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
  
  QString getContentText(ToonzScene*);

public:
  BoardItem();

  QRectF getItemRect(QSize imgSize); //�摜���Rect��Ԃ�
  void drawItem(QPainter& p, QSize imgSize, int shrink, ToonzScene* scene);

};

class DVAPI BoardSettings {
  //�L������
  bool m_active = true;
  //�t���[����
  int m_duration = 24;
  //�w�i�摜
  TFilePath m_bgPath;
  //�e�p�[�c
  QList<BoardItem> m_items;

public:
  BoardSettings();

  TRaster32P getBoardRaster(TDimension& dim, int shrink, ToonzScene* scene);

  int getDuration() {
    return (m_active) ? m_duration : 0;
  }
};

#endif