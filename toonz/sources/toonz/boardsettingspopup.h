#pragma once

#ifndef BOARDSETTINGSPOPUP_H
#define BOARDSETTINGSPOPUP_H

#include "toonzqt/dvdialog.h"
#include "tpixel.h"
#include <QWidget>
#include <QStackedWidget>

class TOutputProperties;
class QLineEdit;
class QTextEdit;
class QComboBox;
class QFontComboBox;
class QListWidget;
class BoardItem;

namespace DVGui {
  class FileField;
  class ColorField;
  class IntLineEdit;
}

//=============================================================================

class BoardView : public QWidget {
  Q_OBJECT

  enum DragItem {
    None = 0,
    Translate,
    TopLeftCorner,
    TopRightCorner,
    BottomRightCorner,
    BottomLeftCorner,
    TopEdge,
    RightEdge,
    BottomEdge,
    LeftEdge
  } m_dragItem = None;

  QImage m_boardImg;
  bool m_valid = false;

  QRectF m_boardImgRect;

  QRectF  m_dragStartItemRect;
  QPointF m_dragStartPos;



public:
  BoardView(QWidget* parent = nullptr);
  void invalidate() { m_valid = false; }
protected:
  void paintEvent(QPaintEvent *event);
  void resizeEvent(QResizeEvent* event);

  void mouseMoveEvent(QMouseEvent *event);
  void mousePressEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);
};

//=============================================================================

class ItemInfoView : public QStackedWidget {
  Q_OBJECT

  //BoardItem* m_currentItem = nullptr;

  QLineEdit * m_nameEdit;
  DVGui::IntLineEdit *m_maxFontSizeEdit;
  QComboBox * m_typeCombo;
  QTextEdit * m_textEdit;
  DVGui::FileField * m_imgPathField;
  QFontComboBox* m_fontCombo;
  QPushButton* m_boldButton, *m_italicButton;
  DVGui::ColorField * m_fontColorField;

  QWidget* m_fontPropBox;//これをまとめてONOFFする
  
public:
  ItemInfoView(QWidget* parent = nullptr);

  //アイテムが切り替わったとき、表示を更新
  void setCurrentItem(int index);

protected slots:
  void onNameEdited();
  void onMaxFontSizeEdited();
  void onTypeComboActivated(int);
  void onFreeTextChanged();
  void onImgPathChanged();
  void onFontComboChanged(const QFont &);
  void onBoldButtonClicked(bool);
  void onItalicButtonClicked(bool);
  void onFontColorChanged(const TPixel32 &, bool);

signals:
  void itemPropertyChanged(bool updateListView); // if true update the list view as well

};

//=============================================================================

class ItemListView : public QWidget {
  Q_OBJECT
  QListWidget* m_list;
  QPushButton* m_deleteItemBtn, * m_moveUpBtn, * m_moveDownBtn;
public:
  ItemListView(QWidget* parent = nullptr);
  void initialize();

  // infoViewの内容が編集されたとき、同期する
  void updateCurrentItem();
protected slots:
  void onCurrentItemSwitched(int);
  void onNewItemButtonClicked();
  void onDeleteItemButtonClicked();
signals:
  void currentItemSwitched(int);
  void itemAddedOrDeleted();
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

  static BoardItem* currentBoardItem;

  BoardSettingsPopup(QWidget *parent = nullptr);
protected:
  void showEvent(QShowEvent*) { initialize(); }
protected slots:
  void onCurrentItemSwitched(int);
  void onItemAddedOrDeleted();
  void onItemPropertyChanged(bool updateListView);
};



#endif
