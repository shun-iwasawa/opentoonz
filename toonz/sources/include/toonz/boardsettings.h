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
    FreeText = 0,     //自由テキスト (m_textに内容)
    ProjectName,      //プロジェクト名
    SceneName,        //シーンファイル名
    Duration_Frame,   //長さ（フレーム数）
    Duration_SecFrame,//長さ（秒＋コマ）
    Duration_HHMMSSFF,//長さ（HH:MM:SS:FF）
    CurrentDate,      //現在の日（年 / 月 / 日）
    CurrentDateTime,  //現在の日時（年 / 月 / 日 / 時 / 分 / 秒）
    UserName,         //ユーザ名
    ScenePath_Aliased,//シーンファイルパス（エイリアス付）
    ScenePath_Full,   //シーンファイルパス（フルパス）
    MoviePath_Aliased,//ムービーファイルパス（エイリアス付）
    MoviePath_Full,   //ムービーファイルパス（フルパス）
    Image             //画像 (m_imgPathにパス)
  };

private:
  QString m_name;
  Type m_type;

  // 画面全体に対する％表記
  QRectF m_rect;
  
  // 文字の最大サイズ
  int m_maximumFontSize;

  // 文字の色
  QColor m_color;

  // フォント
  QFont m_font;

  QString m_text;
  
  QString getContentText(ToonzScene*);

public:
  BoardItem();

  QRectF getItemRect(QSize imgSize); //画像上のRectを返す
  void drawItem(QPainter& p, QSize imgSize, int shrink, ToonzScene* scene);

  QString getName() { return m_name; }
  void setname(QString name) { m_name = name; }

  Type getType() { return m_type; }
  void setType(Type type) { m_type = type; }

  int getMaximumFontSize() { return m_maximumFontSize; }
  void setMaximumFontSize(int size) { m_maximumFontSize = size; }

  QColor getColor() { return m_color; }
  void setColor(QColor color) { m_color = color; }

  QFont& font() { return m_font; }

  QString getFreeText() { return m_text; }
  void setFreeText(QString text) { m_text = text; }
};

class DVAPI BoardSettings {
  //有効無効
  bool m_active = true;
  //フレーム長
  int m_duration = 24;
  //背景画像
  TFilePath m_bgPath;
  //各パーツ
  QList<BoardItem> m_items;

public:
  BoardSettings();

  QImage getBoardImage(TDimension& dim, int shrink, ToonzScene* scene);

  TRaster32P getBoardRaster(TDimension& dim, int shrink, ToonzScene* scene);

  int getDuration() {
    return (m_active) ? m_duration : 0;
  }

  bool isActive() { return m_active; }
  void setActive(bool on) { m_active = on; }

  int getItemCount() { return m_items.count(); }
  BoardItem& getItem(int index) { return m_items[index]; }

  void setDuration(int f) { m_duration = f; }

  TFilePath getBgPath() { return m_bgPath; }
  void setBgPath(TFilePath path) { m_bgPath = path; }
};

#endif