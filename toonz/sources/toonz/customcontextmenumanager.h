#pragma once

#ifndef CUSTOMCONTEXTMENUMANAGER_H
#define CUSTOMCONTEXTMENUMANAGER_H

#include <QObject>
#include <QWidget>
#include <QMap>
#include <QVariant>
#include <QTreeWidget>
#include <QListWidget>
#include <QStyledItemDelegate>
#include "toonzqt/dvdialog.h"
#include "commandbarpopup.h"
#include "tfilepath.h"

class QMenu;
class QXmlStreamReader;
class QXmlStreamWriter;
class CommandListTree;

enum CONDITION_MASKS {
  NO_Condition = 0x0,
  Condition01  = 0x1,
  Condition02  = 0x2,
  Condition03  = 0x4,
  Condition04  = 0x8,
  Condition05  = 0x10,
  Condition06  = 0x20,
  Condition07  = 0x40,
  Condition08  = 0x80,
  Condition09  = 0x100,
  Condition10  = 0x200,
  Condition11  = 0x400,
  Condition12  = 0x800,
  Condition13  = 0x1000,
  Condition14  = 0x2000,
  Condition15  = 0x4000,
  Condition16  = 0x8000,
  Condition17  = 0x10000,
  Condition18  = 0x20000,
  Condition19  = 0x40000,
  Condition20  = 0x80000
};

//-----------------------------------------------------------------------------

// デフォルトメニュー登録、メニュー情報からメニューを生成
class CustomContextMenuWidget : public QWidget {
  Q_OBJECT
  QString m_widgetId;
  QString m_translateContext;

protected:
  QVariant m_contextMenuData;

public:
  CustomContextMenuWidget(const QString& widgetId,
                          const QString& translateContext,
                          QWidget* parent   = nullptr,
                          Qt::WindowFlags f = Qt::WindowFlags());
  void registerMenu(
      QMenu* menu, const QString& type = QString(),
      QMap<QString, QString>&         = QMap<QString, QString>{},
      QMap<CONDITION_MASKS, QString>& = QMap<CONDITION_MASKS, QString>{});
  // メニュー情報からメニューを生成
  void getMenu(QMenu& menu, const QString& type = QString(),
               unsigned int mask = 0);
  void doGetMenu(QMenu& menu, const QMenu* menuInfo, unsigned int mask);

  virtual QAction* customContextAction(const QString& cmdId) { return nullptr; }
  virtual void registerContextMenus(){};
protected slots:
  void openCustomizeContextMenuPopup();
};
//-----------------------------------------------------------------------------

// デフォルト／カスタムメニューの登録、設定ファイルからの読み出し、書き込み
class CustomContextMenuManager : public QObject {  // singleton
  Q_OBJECT

  struct CustomMenuData {
    QMenu* defaultMenu;
    QMenu* customMenu = nullptr;
    QMap<QString, QString> specialCommandLabels;
    QMap<CONDITION_MASKS, QString> conditionDescriptions;
  };

  QMap<QString, QMap<QString, CustomMenuData>> m_customMenuMap;
  QMap<QString, QString> m_translateContextMap;
  QMap<CONDITION_MASKS, QPixmap> m_conditionIcons;

  CustomContextMenuManager() {}

public:
  // メニューをdelete
  ~CustomContextMenuManager();
  static CustomContextMenuManager* instance();
  // メニュー情報登録、カスタムメニューをファイルから読み出し
  void registerMenu(
      const QString& widgetId, const QString& type, QMenu* menu,
      const QString& translateContext,
      QMap<QString, QString>&         = QMap<QString, QString>{},
      QMap<CONDITION_MASKS, QString>& = QMap<CONDITION_MASKS, QString>{});
  void updateCustomMenu(const QString& widgetId, const QString& type);
  void deleteCustomMenu(const QString& widgetId, const QString& type);

  // メニュー情報取得
  QMenu* getMenu(const QString& widgetId, const QString& type);
  // 固有コマンドのラベル一覧取得
  const QMap<QString, QString>& getSpecialCommandLabels(const QString& widgetId,
                                                        const QString& type);
  // メニュー表示条件の説明文一覧取得
  const QMap<CONDITION_MASKS, QString>& getConditionDescriptions(
      const QString& widgetId, const QString& type);

  bool isRegistered(const QString& widgetId);

  QPixmap getCondIcon(CONDITION_MASKS maskId);
  TFilePath getSettingsPath(const QString& widgetId, const QString& type);
  QMenu* loadMenuTree(const TFilePath& fp);
  void loadMenuRecursive(QXmlStreamReader& reader, QMenu* menu);
  QString getTranslteContext(QString widgetId) const;
};

//=============================================================================
// ContextMenuTree
//-----------------------------------------------------------------------------

class ContextMenuTree final : public QTreeWidget {
  Q_OBJECT

  TFilePath m_path;
  QString m_widgetId;
  QString m_type;

  void saveMenuRecursive(QXmlStreamWriter& writer, QTreeWidgetItem* parentItem);

  void buildMenuTreeRecursive(const QMenu*, QTreeWidgetItem* parentItem);

  void buildMenuTree(const QMenu*);

public:
  ContextMenuTree(const QString& widgetId, const QString& type,
                  QWidget* parent = 0);
  void saveMenuTree();
  QAction* actionFromIndex(const QModelIndex& index, QString& text) const;
  void resetToDefault();

protected:
  bool dropMimeData(QTreeWidgetItem* parent, int index, const QMimeData* data,
                    Qt::DropAction action) override;
  QStringList mimeTypes() const override;
  void contextMenuEvent(QContextMenuEvent* event) override;
  void dragEnterEvent(QDragEnterEvent*) override;
protected slots:
  void insertMenu();
  void removeItem();
  void onToggleCondition();
};

//=============================================================================
// ConditionList
//-----------------------------------------------------------------------------

class ConditionList final : public QListWidget {
  Q_OBJECT

  QString m_widgetId;
  QString m_type;

public:
  ConditionList(const QString& widgetId, const QString& type,
                QWidget* parent = 0);

protected:
  void mousePressEvent(QMouseEvent*) override;
};

//=============================================================================
// ContextMenuTreeItem
//-----------------------------------------------------------------------------

class ContextMenuTreeItem : public QTreeWidgetItem {
  QAction* m_action;

public:
  ContextMenuTreeItem(QTreeWidgetItem* parent, QAction* action)
      : QTreeWidgetItem(parent, UserType), m_action(action) {}

  QAction* getRegAction() const { return m_action; }
  void addCondition(uint maskId);
  void toggleCondition(uint maskId);
};

//=============================================================================
// ContextMenuSubmenuItem
//-----------------------------------------------------------------------------

class ContextMenuSubmenuItem final : public ContextMenuTreeItem {
public:
  ContextMenuSubmenuItem(QTreeWidgetItem* parent, QAction* action,
                         QString& title);
};

//=============================================================================
// ContextMenuCommandItem
//-----------------------------------------------------------------------------

class ContextMenuCommandItem final : public ContextMenuTreeItem {
  QAction* m_cmdAction;

public:
  ContextMenuCommandItem(QTreeWidgetItem* parent, QAction* cmdAction,
                         QAction* regAction);
  QAction* getCmdAction() const { return m_cmdAction; }
};

//=============================================================================
// ContextMenuSeparatorItem
//-----------------------------------------------------------------------------

class ContextMenuSeparatorItem final : public ContextMenuTreeItem {
public:
  ContextMenuSeparatorItem(QTreeWidgetItem* parent, QAction* action);
};

//=============================================================================
// CustomizeContextMenuPopup
//-----------------------------------------------------------------------------

class CustomizeContextMenuPopup final : public DVGui::Dialog {
  Q_OBJECT
  CommandListTree* m_commandListTree;
  ContextMenuTree* m_contextMenuTree;
  ConditionList* m_conditionList;

public:
  CustomizeContextMenuPopup(const QString& widgetId, const QString& type);
protected slots:
  void onOkPressed();
  void onReset();
  void onSearchTextChanged(const QString& text);
};

//-----------------------------------------------------------------------------

class ContextMenuTreeDelegate : public QStyledItemDelegate {
  Q_OBJECT

public:
  ContextMenuTreeDelegate(QObject* parent) : QStyledItemDelegate(parent) {}

  void paint(QPainter* painter, const QStyleOptionViewItem& option,
             const QModelIndex& index) const override;
};

#endif