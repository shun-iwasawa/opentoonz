#pragma once

#ifndef BOARDSETTINGSPOPUP_H
#define BOARDSETTINGSPOPUP_H

#include "toonzqt/dvdialog.h"
#include <QWidget>

class TOutputProperties;
class QLineEdit;
class QTextEdit;
class QComboBox;
class QFontComboBox;
class QListWidget;
class BoardItem;

namespace DVGui {
  class FileField;
  class StyleSample;
  class IntLineEdit;
}

//=============================================================================

class BoardView : public QWidget {
  Q_OBJECT

  QImage m_boardImg;
  bool m_valid = false;

  QRectF m_boardImgRect;

public:
  BoardView(QWidget* parent = nullptr) : QWidget(parent) {}
  void invalidate() { m_valid = false; }
protected:
  void paintEvent(QPaintEvent *event);
  void resizeEvent(QResizeEvent* event);
};

//=============================================================================

class ItemInfoView : public QWidget {
  Q_OBJECT

  BoardItem* m_currentItem = nullptr;

  QLineEdit * m_nameEdit, * m_maxFontSizeEdit;
  QComboBox * m_typeCombo;
  QTextEdit * m_textEdit;
  DVGui::FileField * m_imgPathField;
  QFontComboBox* m_fontCombo;
  QPushButton* m_boldButton, *m_italicButton;
  DVGui::StyleSample * m_fontColorSample;

  QWidget* m_fontPropBox;//これをまとめてONOFFする
  
public:
  ItemInfoView(QWidget* parent = nullptr);

  //アイテムが切り替わったとき、表示を更新

};

//=============================================================================

class ItemListView : public QWidget {
  Q_OBJECT
  QListWidget* m_list;
  QPushButton* m_moveUpBtn, * m_moveDownBtn;
public:
  ItemListView(QWidget* parent = nullptr);
  void initialize();
};

//=============================================================================

class BoardSettingsPopup : public DVGui::Dialog {
  Q_OBJECT
    
  BoardView* m_boardView;
  ItemInfoView* m_itemInfoView;
  ItemListView* m_itemListView;

  DVGui::IntLineEdit* m_durationEdit;
  DVGui::FileField * m_backgroundPathField;
  
  void initialize();
  void initializeItemTypeString(); // call once on the first launch
public:
  BoardSettingsPopup(QWidget *parent = nullptr);
protected:
  void showEvent(QShowEvent*) { initialize(); }
};



#endif
