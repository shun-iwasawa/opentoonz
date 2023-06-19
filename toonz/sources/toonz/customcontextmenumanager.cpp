#include "customcontextmenumanager.h"

#include "toonzqt/menubarcommand.h"
#include "toonz/toonzfolders.h"
#include "tsystem.h"
// TnzQt includes
#include "toonzqt/gutil.h"
#include "tapp.h"

#include <QMenu>
#include <QHeaderView>
#include <QApplication>
#include <QXmlStreamReader>
#include <QMimeData>
#include <QDebug>
#include <QContextMenuEvent>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QMainWindow>
#include <QPainter>
#include <QDrag>
#include <QDragEnterEvent>
#include <QSplitter>
#include <QMessageBox>

namespace {
int count_bits(unsigned int n) {
  n = (n & 0x55555555) + (n >> 1 & 0x55555555);
  n = (n & 0x33333333) + (n >> 2 & 0x33333333);
  n = (n & 0x0f0f0f0f) + (n >> 4 & 0x0f0f0f0f);
  n = (n & 0x00ff00ff) + (n >> 8 & 0x00ff00ff);
  n = (n & 0x0000ffff) + (n >> 16 & 0x0000ffff);
  return n;
}
}  // namespace
//-----------------------------------------------------------------

CustomContextMenuWidget::CustomContextMenuWidget(
    const QString& widgetId, const QString& translateContext, QWidget* parent,
    Qt::WindowFlags f)
    : QWidget(parent, f)
    , m_widgetId(widgetId)
    , m_translateContext(translateContext) {}

//-----------------------------------------------------------------

void CustomContextMenuWidget::registerMenu(
    QMenu* menu, const QString& type,
    QMap<QString, QString>& specialCommandLabels,
    QMap<CONDITION_MASKS, QString>& conditionDescriptions) {
  CustomContextMenuManager::instance()->registerMenu(
      m_widgetId, type, menu, m_translateContext, specialCommandLabels,
      conditionDescriptions);
}

//-----------------------------------------------------------------
// メニュー情報からメニューを生成
void CustomContextMenuWidget::getMenu(QMenu& menu, const QString& type,
                                      unsigned int mask) {
  // とりあえずここに置くけど、時間がかかるようならWidget作成時にするかも
  // build context menus for the first time
  if (!CustomContextMenuManager::instance()->isRegistered(m_widgetId))
    registerContextMenus();

  // メニュー情報取得
  QMenu* menuInfo =
      CustomContextMenuManager::instance()->getMenu(m_widgetId, type);
  doGetMenu(menu, menuInfo, mask);

  // 最後にカスタマイズボタンを追加
  menu.addSeparator();
  QAction* customizeCmd = menu.addAction(tr("Customize Menu..."));
  QStringList idList    = {m_widgetId, type};
  customizeCmd->setData(idList);  // 可読情報にしたい。要検討

  connect(customizeCmd, SIGNAL(triggered()), this,
          SLOT(openCustomizeContextMenuPopup()));
}

//-----------------------------------------------------------------

void CustomContextMenuWidget::openCustomizeContextMenuPopup() {
  QAction* action    = qobject_cast<QAction*>(sender());
  QStringList idList = action->data().toStringList();
  CustomizeContextMenuPopup popup(idList[0], idList[1]);

  if (popup.exec()) {
    /*- OKが押され、設定ファイル.xmlが更新された状態 -*/
    /*- xmlファイルからカスタムメニューを作り格納 -*/
  }
}

//-----------------------------------------------------------------

void CustomContextMenuWidget::doGetMenu(QMenu& menu, const QMenu* menuInfo,
                                        unsigned int mask) {
  for (auto a_info : menuInfo->actions()) {
    QString a_id        = a_info->text();
    unsigned int a_mask = a_info->data().toUInt();

    if (a_mask != 0 && (a_mask & mask) != a_mask) continue;

    // menu case
    if (a_info->menu()) {
      QMenu* subMenu =
          menu.addMenu(qApp->translate(m_translateContext.toStdString().c_str(),
                                       a_id.toStdString().c_str()));
      doGetMenu(*subMenu, a_info->menu(), mask);
      continue;
    }
    // separator case
    if (a_info->isSeparator()) {
      menu.addSeparator();
      continue;
    }
    // command case
    QAction* action =
        CommandManager::instance()->getAction(a_id.toStdString().c_str());
    // menu command case
    if (action) {
      menu.addAction(action);
    }
    // original command case
    else {
      QAction* customAction = customContextAction(a_id);
      assert(customAction);
      menu.addAction(customAction);
    }
  }
}

//-----------------------------------------------------------------
//-----------------------------------------------------------------

CustomContextMenuManager* CustomContextMenuManager::instance() {
  static CustomContextMenuManager _instance;
  return &_instance;
}

//-----------------------------------------------------------------
// メニューをdelete
CustomContextMenuManager::~CustomContextMenuManager() {
  for (auto dataPerWidget : m_customMenuMap.values()) {
    for (auto data : dataPerWidget.values()) {
      delete data.defaultMenu;
      delete data.customMenu;
    }
  }
}

//-----------------------------------------------------------------
// メニュー情報登録、カスタムメニューをファイルから読み出し
void CustomContextMenuManager::registerMenu(
    const QString& widgetId, const QString& type, QMenu* menu,
    const QString& translateContext,
    QMap<QString, QString>& specialCommandLabels,
    QMap<CONDITION_MASKS, QString>& conditionDescriptions) {
  CustomMenuData data;
  data.defaultMenu           = menu;
  data.specialCommandLabels  = specialCommandLabels;
  data.conditionDescriptions = conditionDescriptions;
  if (!m_customMenuMap.contains(widgetId)) {
    QMap<QString, CustomMenuData> dataPerWidget;
    m_customMenuMap.insert(widgetId, dataPerWidget);
  }
  if (!m_translateContextMap.contains(widgetId)) {
    m_translateContextMap.insert(widgetId, translateContext);
  }

  // 設定ファイルを確認
  TFilePath settingsPath = getSettingsPath(widgetId, type);
  if (!TFileStatus(settingsPath).doesExist()) {
    m_customMenuMap[widgetId].insert(type, data);
    return;
  }

  // 設定ファイルを読み込んでcustomMenuに入れる
  data.customMenu = loadMenuTree(settingsPath);
  m_customMenuMap[widgetId].insert(type, data);
}

//-----------------------------------------------------------------

void CustomContextMenuManager::updateCustomMenu(const QString& widgetId,
                                                const QString& type) {
  if (m_customMenuMap[widgetId][type].customMenu)
    delete m_customMenuMap[widgetId][type].customMenu;

  TFilePath settingsPath = getSettingsPath(widgetId, type);
  assert(TFileStatus(settingsPath).doesExist());

  m_customMenuMap[widgetId][type].customMenu = loadMenuTree(settingsPath);
}

//-----------------------------------------------------------------

void CustomContextMenuManager::deleteCustomMenu(const QString& widgetId,
                                                const QString& type) {
  if (m_customMenuMap[widgetId][type].customMenu) {
    delete m_customMenuMap[widgetId][type].customMenu;
    m_customMenuMap[widgetId][type].customMenu = nullptr;
  }

  TFilePath settingsPath = getSettingsPath(widgetId, type);
  if (!TFileStatus(settingsPath).doesExist()) return;
  TSystem::deleteFile(settingsPath);
}

//-----------------------------------------------------------------
// メニュー情報取得
QMenu* CustomContextMenuManager::getMenu(const QString& widgetId,
                                         const QString& type) {
  assert(m_customMenuMap.contains(widgetId) &&
         m_customMenuMap[widgetId].contains(type));

  CustomMenuData data = m_customMenuMap[widgetId][type];
  if (data.customMenu) return data.customMenu;
  return data.defaultMenu;
}

//-----------------------------------------------------------------
// 固有コマンドのラベル一覧取得
const QMap<QString, QString>& CustomContextMenuManager::getSpecialCommandLabels(
    const QString& widgetId, const QString& type) {
  return m_customMenuMap[widgetId][type].specialCommandLabels;
}

//-----------------------------------------------------------------
// メニュー表示条件の説明文一覧取得
const QMap<CONDITION_MASKS, QString>&
CustomContextMenuManager::getConditionDescriptions(const QString& widgetId,
                                                   const QString& type) {
  return m_customMenuMap[widgetId][type].conditionDescriptions;
}

//-----------------------------------------------------------------

bool CustomContextMenuManager::isRegistered(const QString& widgetId) {
  return m_customMenuMap.contains(widgetId);
}

//-----------------------------------------------------------------

QPixmap CustomContextMenuManager::getCondIcon(CONDITION_MASKS maskId) {
  if (m_conditionIcons.contains(maskId)) return m_conditionIcons.value(maskId);

  static QList<QColor> colorTable = {
      QColor(167, 55, 55),  QColor(195, 115, 40), QColor(214, 183, 22),
      QColor(165, 179, 57), QColor(82, 157, 79),  QColor(71, 142, 165),
      QColor(64, 103, 172), QColor(60, 49, 187),  QColor(108, 66, 170),
      QColor(161, 75, 140), QColor(111, 29, 108), QColor(255, 255, 255)};

  int idNum        = (int)std::round(std::log2((int)maskId));
  QColor iconColor = colorTable[idNum % 12];

  QColor textColor = (iconColor.valueF() > 0.5) ? Qt::black : Qt::white;

  QPixmap pm(12, 12);
  pm.fill(Qt::transparent);
  QPainter p(&pm);
  p.setPen(Qt::NoPen);
  p.setBrush(iconColor);
  p.drawEllipse(0, 0, 12, 12);
  p.setPen(textColor);
  p.drawText(0, 0, 12, 12, Qt::AlignCenter, QString::number(idNum + 1));

  m_conditionIcons.insert(maskId, pm);
  return pm;
}

//-----------------------------------------------------------------

TFilePath CustomContextMenuManager::getSettingsPath(const QString& widgetId,
                                                    const QString& type) {
  std::string settingsFileName;
  if (type.isEmpty())
    settingsFileName = widgetId.toStdString() + ".xml";
  else
    settingsFileName =
        widgetId.toStdString() + "_" + type.toStdString() + ".xml";
  return ToonzFolder::getMyModuleDir() + "custom_context_menus" +
         settingsFileName;
}

//-----------------------------------------------------------------

QMenu* CustomContextMenuManager::loadMenuTree(const TFilePath& fp) {
  QFile file(toQString(fp));
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    qDebug() << "Cannot read file" << file.errorString();
    return nullptr;
  }

  QMenu* menu = new QMenu();
  QXmlStreamReader reader(&file);

  if (reader.readNextStartElement()) {
    if (reader.name() == "context_menu") {
      loadMenuRecursive(reader, menu);
    } else
      reader.raiseError(QObject::tr("Incorrect file"));
  }

  if (reader.hasError()) {
    qDebug() << "Cannot read menubar xml";
  }

  return menu;
}

//-----------------------------------------------------------------------------
QMutex mutex;
void CustomContextMenuManager::loadMenuRecursive(QXmlStreamReader& reader,
                                                 QMenu* menu) {
  while (reader.readNextStartElement()) {
    if (reader.name() == "menu") {
      QString untranslatedTitle = reader.attributes().value("title").toString();
      uint mask = reader.attributes().value("condition").toUInt();
      std::cout << "untranslatedTitle = " << untranslatedTitle.toStdString()
                << std::endl;
      QMenu* subMenu;
      {
        QMutexLocker locker(&mutex);
        subMenu = menu->addMenu(untranslatedTitle);
      }
      subMenu->menuAction()->setIconText(untranslatedTitle);
      subMenu->menuAction()->setData(mask);
      loadMenuRecursive(reader, subMenu);
    } else if (reader.name() == "command") {
      uint mask       = reader.attributes().value("condition").toUInt();
      QString cmdName = reader.readElementText();
      std::cout << "command " << cmdName.toStdString()
                << "   condition = " << mask << std::endl;
      {
        QMutexLocker locker(&mutex);
        menu->addAction(cmdName)->setData(mask);
      }
    } else if (reader.name() == "separator") {
      uint mask = reader.attributes().value("condition").toUInt();
      QAction* sepAct;
      {
        QMutexLocker locker(&mutex);
        sepAct = menu->addSeparator();
      }
      sepAct->setData(mask);
      reader.skipCurrentElement();
    } else
      reader.skipCurrentElement();
  }
}

//-----------------------------------------------------------------------------

QString CustomContextMenuManager::getTranslteContext(QString widgetId) const {
  return m_translateContextMap.value(widgetId, "QObject");
}

//=============================================================================
// ContextMenuTreeItem
//-----------------------------------------------------------------------------

void ContextMenuTreeItem::addCondition(uint maskId) {
  bool ok;
  uint mask = m_action->data().toUInt(&ok);
  if (!ok) mask = 0;
  mask = mask | maskId;
  m_action->setData(mask);
}

void ContextMenuTreeItem::toggleCondition(uint maskId) {
  bool ok;
  uint mask = m_action->data().toUInt(&ok);
  if (!ok) mask = 0;
  if (mask & maskId)
    mask = mask & ~maskId;
  else
    mask = mask | maskId;
  m_action->setData(mask);
}

//=============================================================================
// ContextMenuOriginalCommandItem
//-----------------------------------------------------------------------------

class ContextMenuOriginalCommandItem final : public ContextMenuTreeItem {
public:
  ContextMenuOriginalCommandItem(QTreeWidgetItem* parent, QAction* action,
                                 const QString& label)
      : ContextMenuTreeItem(parent, action) {
    setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled |
             Qt::ItemNeverHasChildren);

    setText(0, label);
    setToolTip(0, QObject::tr("[Drag] to move position"));
  }
};

//=============================================================================
// ContextMenuSubmenuItem
//-----------------------------------------------------------------------------
ContextMenuSubmenuItem::ContextMenuSubmenuItem(QTreeWidgetItem* parent,
                                               QAction* action, QString& title)
    : ContextMenuTreeItem(parent, action) {
  setFlags(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsDragEnabled |
           Qt::ItemIsDropEnabled | Qt::ItemIsEnabled);
  /*- Menu title will be translated if the title is registered in translation
   * file -*/
  setText(0, title);
  QIcon subMenuIcon(createQIcon("folder", true));
  setIcon(0, subMenuIcon);
  setToolTip(
      0, QObject::tr("[Drag] to move position, [Double Click] to edit title"));
}

//=============================================================================
// ContextMenuCommandItem
//-----------------------------------------------------------------------------
ContextMenuCommandItem::ContextMenuCommandItem(QTreeWidgetItem* parent,
                                               QAction* cmdAction,
                                               QAction* regAction)
    : ContextMenuTreeItem(parent, regAction), m_cmdAction(cmdAction) {
  setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled |
           Qt::ItemNeverHasChildren);
  QString tempText = m_cmdAction->text();
  // removing accelerator key indicator
  tempText = tempText.replace(QRegExp("&([^& ])"), "\\1");
  // removing doubled &s
  tempText = tempText.replace("&&", "&");
  setText(0, tempText);
  setToolTip(0, QObject::tr("[Drag] to move position"));
}

//=============================================================================
// ContextMenuSeparatorItem
//-----------------------------------------------------------------------------

ContextMenuSeparatorItem::ContextMenuSeparatorItem(QTreeWidgetItem* parent,
                                                   QAction* action)
    : ContextMenuTreeItem(parent, action) {
  setFlags(Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled |
           Qt::ItemNeverHasChildren);
  setText(0, QObject::tr("----Separator----"));
  setToolTip(0, QObject::tr("[Drag] to move position"));
}

//=============================================================================
// ContextMenuTree
//-----------------------------------------------------------------------------

ContextMenuTree::ContextMenuTree(const QString& widgetId, const QString& type,
                                 QWidget* parent)
    : QTreeWidget(parent), m_widgetId(widgetId), m_type(type) {
  ContextMenuTreeDelegate* delegate = new ContextMenuTreeDelegate(this);
  setItemDelegate(delegate);

  m_path =
      CustomContextMenuManager::instance()->getSettingsPath(widgetId, type);

  setObjectName("SolidLineFrame");
  setAlternatingRowColors(true);
  setDragEnabled(true);
  setDropIndicatorShown(true);
  setDefaultDropAction(Qt::MoveAction);
  setDragDropMode(QAbstractItemView::DragDrop);
  setIconSize(QSize(21, 18));

  setColumnCount(1);
  header()->close();

  buildMenuTree(CustomContextMenuManager::instance()->getMenu(widgetId, type));
  // if (TFileStatus(m_path).isWritable()) {
  //   loadMenuTree(m_path);
  // }
  // else {
  //   //fp = m_path.withParentDir(ToonzFolder::getTemplateRoomsDir());
  //   //if (!TFileStatus(path).isReadable())
  //   //  fp = ToonzFolder::getTemplateRoomsDir() + "menubar_template.xml";
  // }
}

//-----------------------------------------------------------------------------
/*
void ContextMenuTree::loadMenuTree(const TFilePath& fp) {
  QFile file(toQString(fp));
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    qDebug() << "Cannot read file" << file.errorString();
    return;
  }

  QXmlStreamReader reader(&file);

  if (reader.readNextStartElement()) {
    if (reader.name() == "menubar") {
      while (reader.readNextStartElement()) {
        if (reader.name() == "menu") {
          QString title = reader.attributes().value("title").toString();
          ContextMenuSubmenuItem* menu = new ContextMenuSubmenuItem(0, title);
          addTopLevelItem(menu);
          loadMenuRecursive(reader, menu);
        }
        else if (reader.name() == "command") {
          QString cmdName = reader.readElementText();

          QAction* action = CommandManager::instance()->getAction(
            cmdName.toStdString().c_str());
          if (action) {
            CommandItem* item = new CommandItem(0, action);
            addTopLevelItem(item);
          }
        }
        else
          reader.skipCurrentElement();
      }
    }
    else
      reader.raiseError(QObject::tr("Incorrect file"));
  }

  if (reader.hasError()) {
    qDebug() << "Cannot read menubar xml";
  }
}

//-----------------------------------------------------------------------------

void ContextMenuTree::loadMenuRecursive(QXmlStreamReader& reader,
  QTreeWidgetItem* parentItem) {
  while (reader.readNextStartElement()) {
    if (reader.name() == "menu") {
      QString title = reader.attributes().value("title").toString();
      ContextMenuSubmenuItem* subMenu = new ContextMenuSubmenuItem(parentItem,
title); loadMenuRecursive(reader, subMenu);
    }
    else if (reader.name() == "command") {
      QString cmdName = reader.readElementText();
      QAction* action =
        CommandManager::instance()->getAction(cmdName.toStdString().c_str());
      if (action) CommandItem* item = new CommandItem(parentItem, action);
    }
    else if (reader.name() == "command_debug") {
#ifndef NDEBUG
      QString cmdName = reader.readElementText();
      QAction* action =
        CommandManager::instance()->getAction(cmdName.toStdString().c_str());
      if (action) CommandItem* item = new CommandItem(parentItem, action);
#else
      reader.skipCurrentElement();
#endif
    }
    else if (reader.name() == "separator") {
      SeparatorItem* sep = new SeparatorItem(parentItem);
      reader.skipCurrentElement();
    }
    else
      reader.skipCurrentElement();
  }
}

*/

//-----------------------------------------------------------------------------

void ContextMenuTree::saveMenuRecursive(QXmlStreamWriter& writer,
                                        QTreeWidgetItem* parentItem) {
  for (int c = 0; c < parentItem->childCount(); c++) {
    ContextMenuCommandItem* command =
        dynamic_cast<ContextMenuCommandItem*>(parentItem->child(c));
    ContextMenuOriginalCommandItem* origCommand =
        dynamic_cast<ContextMenuOriginalCommandItem*>(parentItem->child(c));
    ContextMenuSeparatorItem* sep =
        dynamic_cast<ContextMenuSeparatorItem*>(parentItem->child(c));
    ContextMenuSubmenuItem* subMenu =
        dynamic_cast<ContextMenuSubmenuItem*>(parentItem->child(c));

    QAction* action = dynamic_cast<ContextMenuTreeItem*>(parentItem->child(c))
                          ->getRegAction();
    bool ok;
    uint condMask = action->data().toUInt(&ok);
    if (!ok) condMask = 0;

    if (command || origCommand) {
      writer.writeStartElement("command");
      if (condMask)
        writer.writeAttribute("condition", QString::number(condMask));
      writer.writeCharacters(action->text());
      writer.writeEndElement();
    } else if (sep) {
      writer.writeEmptyElement("separator");
      if (condMask)
        writer.writeAttribute("condition", QString::number(condMask));
    } else if (subMenu) {
      writer.writeStartElement("menu");
      // save original title instead of translated one
      writer.writeAttribute("title", action->iconText());
      if (condMask)
        writer.writeAttribute("condition", QString::number(condMask));

      saveMenuRecursive(writer, subMenu);

      writer.writeEndElement();  // menu
    }
  }
}

//-----------------------------------------------------------------------------

void ContextMenuTree::buildMenuTree(const QMenu* menu) {
  clear();
  buildMenuTreeRecursive(menu, invisibleRootItem());
}
//-----------------------------------------------------------------------------

void ContextMenuTree::buildMenuTreeRecursive(const QMenu* menu,
                                             QTreeWidgetItem* parentItem) {
  for (auto action : menu->actions()) {
    uint mask = action->data().toUInt();
    if (action->menu()) {
      QString tr_context =
          CustomContextMenuManager::instance()->getTranslteContext(m_widgetId);
      QString title = qApp->translate(tr_context.toStdString().c_str(),
                                      action->text().toStdString().c_str());
      ContextMenuSubmenuItem* menuItem =
          new ContextMenuSubmenuItem(parentItem, action, title);
      buildMenuTreeRecursive(action->menu(), menuItem);
    } else if (action->isSeparator()) {
      ContextMenuSeparatorItem* sep =
          new ContextMenuSeparatorItem(parentItem, action);
    } else {
      QString cmdName = action->text();

      QAction* cmdAction =
          CommandManager::instance()->getAction(cmdName.toStdString().c_str());
      if (cmdAction) {
        ContextMenuCommandItem* item =
            new ContextMenuCommandItem(parentItem, cmdAction, action);
      } else {
        // 固有コマンド
        QMap<QString, QString> labels =
            CustomContextMenuManager::instance()->getSpecialCommandLabels(
                m_widgetId, m_type);
        QString commandLabel = labels.value(action->text(), action->text());

        ContextMenuOriginalCommandItem* item =
            new ContextMenuOriginalCommandItem(parentItem, action,
                                               commandLabel);
      }
    }
  }
}

//-----------------------------------------------------------------------------

void ContextMenuTree::saveMenuTree() {
  if (!TSystem::touchParentDir(m_path)) {
    return;
  }

  QFile file(toQString(m_path));
  if (!file.open(QFile::WriteOnly | QFile::Text)) {
    qDebug() << "Cannot read file" << file.errorString();
    return;
  }
  {
    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();

    writer.writeStartElement("context_menu");
    { saveMenuRecursive(writer, invisibleRootItem()); }
    writer.writeEndElement();  // menubar

    writer.writeEndDocument();
  }
  file.close();
  CustomContextMenuManager::instance()->updateCustomMenu(m_widgetId, m_type);
}

//-----------------------------------------------------------------------------
QAction* ContextMenuTree::actionFromIndex(const QModelIndex& index,
                                          QString& text) const {
  QTreeWidgetItem* item            = itemFromIndex(index);
  text                             = item->text(0);
  ContextMenuTreeItem* commandItem = dynamic_cast<ContextMenuTreeItem*>(item);
  if (commandItem) return commandItem->getRegAction();
  return nullptr;
}

//-----------------------------------------------------------------------------

void ContextMenuTree::resetToDefault() {
  // ファイル消す, メニュー消す
  CustomContextMenuManager::instance()->deleteCustomMenu(m_widgetId, m_type);
  // ツリー再構築
  buildMenuTree(
      CustomContextMenuManager::instance()->getMenu(m_widgetId, m_type));
}

//-----------------------------------------------------------------------------

bool ContextMenuTree::dropMimeData(QTreeWidgetItem* parent, int index,
                                   const QMimeData* data,
                                   Qt::DropAction action) {
  if (!parent) parent = invisibleRootItem();

  if (data->hasText()) {
    QString txt = data->text();

    if (txt.startsWith("CONDITION")) {
      ContextMenuTreeItem* target = dynamic_cast<ContextMenuTreeItem*>(parent);
      if (!target) return false;
      uint maskId = txt.split(' ')[1].toUInt();
      target->addCondition(maskId);
      // update();
      return true;
    }

    QTreeWidgetItem* item;
    if (txt == "separator") {
      QAction* action = new QAction();
      action->setSeparator(true);
      item = new ContextMenuSeparatorItem(nullptr, action);
    } else {
      QAction* action = new QAction(txt);
      QAction* cmdAction =
          CommandManager::instance()->getAction(txt.toStdString().c_str());
      if (!cmdAction) return false;
      item = new ContextMenuCommandItem(nullptr, cmdAction, action);
    }

    parent->insertChild(index, item);

    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------

QStringList ContextMenuTree::mimeTypes() const {
  QStringList qstrList;
  qstrList.append("text/plain");
  return qstrList;
}

//-----------------------------------------------------------------------------

void ContextMenuTree::contextMenuEvent(QContextMenuEvent* event) {
  QTreeWidgetItem* item = itemAt(event->pos());
  if (item != currentItem()) setCurrentItem(item);
  QMenu* menu = new QMenu(this);
  QAction* action;
  if (!item || indexOfTopLevelItem(item) >= 0)
    action = menu->addAction(tr("Insert Menu"));
  else
    action = menu->addAction(tr("Insert Submenu"));

  connect(action, SIGNAL(triggered()), this, SLOT(insertMenu()));

  if (item) {
    action = menu->addAction(tr("Remove \"%1\"").arg(item->text(0)));
    connect(action, SIGNAL(triggered()), this, SLOT(removeItem()));
  }

  // condition
  QMap<CONDITION_MASKS, QString> condDescs =
      CustomContextMenuManager::instance()->getConditionDescriptions(m_widgetId,
                                                                     m_type);
  if (!condDescs.isEmpty()) {
    ContextMenuTreeItem* cmItem = dynamic_cast<ContextMenuTreeItem*>(item);
    if (cmItem) {
      bool ok;
      uint currentMask = cmItem->getRegAction()->data().toUInt(&ok);
      if (!ok) currentMask = 0;

      menu->addSeparator();
      QMenu* conditionsMenu = menu->addMenu(tr("Conditions"));
      for (auto maskId : condDescs.keys()) {
        QString description = condDescs.value(maskId);
        QAction* condAct    = conditionsMenu->addAction(description);
        condAct->setIcon(
            CustomContextMenuManager::instance()->getCondIcon(maskId));
        condAct->setCheckable(true);
        condAct->setChecked(currentMask & maskId);
        condAct->setData(maskId);
        connect(condAct, SIGNAL(triggered()), this, SLOT(onToggleCondition()));
      }
    }
  }

  menu->exec(event->globalPos());
  delete menu;
}
//-----------------------------------------------------------------------------

void ContextMenuTree::dragEnterEvent(QDragEnterEvent* e) {
  bool isCondition = e->mimeData()->text().startsWith("CONDITION");

  QTreeWidgetItemIterator it(this);
  while (*it) {
    if (isCondition)
      (*it)->setFlags((*it)->flags() | Qt::ItemIsDropEnabled);
    else
      (*it)->setFlags((*it)->flags() & ~Qt::ItemIsDropEnabled);

    ++it;
  }
  setDragDropOverwriteMode(isCondition);

  QTreeWidget::dragEnterEvent(e);
}

//-----------------------------------------------------------------------------

void ContextMenuTree::insertMenu() {
  QTreeWidgetItem* item = currentItem();
  QString title         = tr("New Menu");
  QAction* menuAction   = new QAction(title, this);
  ContextMenuSubmenuItem* insItem =
      new ContextMenuSubmenuItem(0, menuAction, title);
  if (!item)
    addTopLevelItem(insItem);
  else if (indexOfTopLevelItem(item) >= 0)
    insertTopLevelItem(indexOfTopLevelItem(item), insItem);
  else
    item->parent()->insertChild(item->parent()->indexOfChild(item), insItem);
}

//-----------------------------------------------------------------------------

void ContextMenuTree::removeItem() {
  QTreeWidgetItem* item = currentItem();
  if (!item) return;

  if (indexOfTopLevelItem(item) >= 0)
    takeTopLevelItem(indexOfTopLevelItem(item));
  else
    item->parent()->removeChild(item);

  delete item;
}

//-----------------------------------------------------------------------------

void ContextMenuTree::onToggleCondition() {
  ContextMenuTreeItem* cmItem =
      dynamic_cast<ContextMenuTreeItem*>(currentItem());
  if (!cmItem) return;

  uint maskId = qobject_cast<QAction*>(sender())->data().toUInt();
  cmItem->toggleCondition(maskId);
}

//=============================================================================
// ConditionList
//-----------------------------------------------------------------------------

ConditionList::ConditionList(const QString& widgetId, const QString& type,
                             QWidget* parent)
    : QListWidget(parent), m_widgetId(widgetId), m_type(type) {
  QMap<CONDITION_MASKS, QString> descriptions =
      CustomContextMenuManager::instance()->getConditionDescriptions(widgetId,
                                                                     type);

  setObjectName("SolidLineFrame");
  setAlternatingRowColors(true);

  if (descriptions.isEmpty()) {
    addItem(tr("- This context menu has no conditions. -"));
    return;
  }

  setDragEnabled(true);
  setDropIndicatorShown(true);
  setDragDropMode(QAbstractItemView::DragOnly);
  setIconSize(QSize(21, 18));

  for (auto maskId : descriptions.keys()) {
    QPixmap pm = CustomContextMenuManager::instance()->getCondIcon(maskId);
    QListWidgetItem* item = new QListWidgetItem(descriptions[maskId], this);
    item->setData(Qt::UserRole, (uint)maskId);
    item->setIcon(QIcon(pm));
    item->setToolTip(QObject::tr("[Drag & Drop] to apply the condition"));
    addItem(item);
  }
}

//-----------------------------------------------------------------------------

void ConditionList::mousePressEvent(QMouseEvent* event) {
  setCurrentItem(itemAt(event->pos()));
  QListWidgetItem* item = itemAt(event->pos());

  if (item) {
    bool ok;
    uint maskId = item->data(Qt::UserRole).toUInt(&ok);
    if (!ok) return;
    QString dragStr = "CONDITION " + QString::number(maskId);

    QMimeData* mimeData = new QMimeData;
    mimeData->setText(dragStr);

    QDrag* drag = new QDrag(this);
    drag->setMimeData(mimeData);
    drag->setPixmap(CustomContextMenuManager::instance()->getCondIcon(
        (CONDITION_MASKS)maskId));
    drag->exec(Qt::CopyAction);
  }

  QListWidget::mousePressEvent(event);
}

//=============================================================================
// CustomizeContextMenuPopup
//-----------------------------------------------------------------------------

CustomizeContextMenuPopup::CustomizeContextMenuPopup(const QString& widgetId,
                                                     const QString& type)
    : Dialog(TApp::instance()->getMainWindow(), true, false,
             "CustomizeContextMenuPopup") {
  QString currentWidgetName;
  if (type.isEmpty())
    currentWidgetName = QString("%1").arg(widgetId);
  else
    currentWidgetName = QString("%1 (%2)").arg(widgetId).arg(type);

  setWindowTitle(tr("Customize Context Menu of \"%1\"").arg(currentWidgetName));

  QLabel* contextMenuLabel =
      new QLabel(tr("%1 Context Menu").arg(currentWidgetName), this);
  QLabel* menuItemListLabel  = new QLabel(tr("Menu Items"), this);
  QLabel* conditionListLabel = new QLabel(tr("Conditions"), this);

  m_commandListTree = new CommandListTree(contextMenuLabel->text(), this);
  m_contextMenuTree = new ContextMenuTree(widgetId, type, this);
  m_conditionList   = new ConditionList(widgetId, type, this);

  QPushButton* resetBtn  = new QPushButton(tr("Reset To Default"), this);
  QPushButton* okBtn     = new QPushButton(tr("OK"), this);
  QPushButton* cancelBtn = new QPushButton(tr("Cancel"), this);

  okBtn->setFocusPolicy(Qt::NoFocus);
  cancelBtn->setFocusPolicy(Qt::NoFocus);

  QFont f("Arial", 15, QFont::Bold);
  contextMenuLabel->setFont(f);
  menuItemListLabel->setFont(f);
  conditionListLabel->setFont(f);

  conditionListLabel->setToolTip(tr(
      "This is a list of conditions for which the command will be displayed.\n"
      "If there are multiple conditions, they are treated as AND conditions.\n"
      "(i.e. The command will be displayed if all of them are true.)"));

  QLineEdit* searchEdit = new QLineEdit(this);

  //--- layout
  QVBoxLayout* mainLay = new QVBoxLayout();
  m_topLayout->setMargin(0);
  m_topLayout->setSpacing(0);
  {
    QGridLayout* mainUILay = new QGridLayout();
    mainUILay->setMargin(5);
    mainUILay->setHorizontalSpacing(8);
    mainUILay->setVerticalSpacing(5);
    {
      mainUILay->addWidget(contextMenuLabel, 0, 0);
      mainUILay->addWidget(menuItemListLabel, 0, 1);
      mainUILay->addWidget(m_contextMenuTree, 1, 0);

      QSplitter* rightSplitter = new QSplitter(this);
      rightSplitter->setOrientation(Qt::Vertical);
      {
        QWidget* commandListWidget  = new QWidget(this);
        QVBoxLayout* commandListLay = new QVBoxLayout();
        commandListLay->setMargin(0);
        commandListLay->setSpacing(5);
        {
          QHBoxLayout* searchLay = new QHBoxLayout();
          searchLay->setMargin(0);
          searchLay->setSpacing(5);
          {
            searchLay->addWidget(new QLabel(tr("Search:"), this), 0);
            searchLay->addWidget(searchEdit);
          }
          commandListLay->addLayout(searchLay, 0);
          commandListLay->addWidget(m_commandListTree, 1);
        }
        commandListWidget->setLayout(commandListLay);
        rightSplitter->addWidget(commandListWidget);

        QWidget* conditionListWidget  = new QWidget(this);
        QVBoxLayout* conditionListLay = new QVBoxLayout();
        conditionListLay->setMargin(0);
        conditionListLay->setSpacing(5);
        {
          conditionListLay->addWidget(conditionListLabel, 0);
          conditionListLay->addWidget(m_conditionList, 1);
        }
        conditionListWidget->setLayout(conditionListLay);
        rightSplitter->addWidget(conditionListWidget);
      }
      rightSplitter->setStretchFactor(0, 2);
      rightSplitter->setStretchFactor(1, 1);
      mainUILay->addWidget(rightSplitter, 1, 1);
    }

    mainUILay->setRowStretch(0, 0);
    mainUILay->setRowStretch(1, 1);
    mainUILay->setColumnStretch(0, 1);
    mainUILay->setColumnStretch(1, 1);

    m_topLayout->addLayout(mainUILay, 1);
  }

  m_buttonLayout->setMargin(0);
  m_buttonLayout->setSpacing(30);
  {
    m_buttonLayout->addStretch(1);
    m_buttonLayout->addWidget(okBtn, 0);
    m_buttonLayout->addWidget(resetBtn, 0);
    m_buttonLayout->addWidget(cancelBtn, 0);
    m_buttonLayout->addStretch(1);
  }

  //--- signal/slot connections

  bool ret = connect(okBtn, SIGNAL(clicked()), this, SLOT(onOkPressed()));
  ret      = ret && connect(resetBtn, SIGNAL(clicked()), this, SLOT(onReset()));
  ret      = ret && connect(cancelBtn, SIGNAL(clicked()), this, SLOT(reject()));
  ret = ret && connect(searchEdit, SIGNAL(textChanged(const QString&)), this,
                       SLOT(onSearchTextChanged(const QString&)));
  assert(ret);
}

//-----------------------------------------------------------------------------

void CustomizeContextMenuPopup::onOkPressed() {
  m_contextMenuTree->saveMenuTree();
  accept();
}

//-----------------------------------------------------------------------------

void CustomizeContextMenuPopup::onReset() {
  QMessageBox::StandardButton ret =
      QMessageBox::question(this, tr("Reset To Default"),
                            tr("Deleting current customization and reset the "
                               "current menu to default.\nAre you sure?"));

  if (ret != QMessageBox::Yes) return;

  // Delete custom menu data and update menu tree
  m_contextMenuTree->resetToDefault();
}

//-----------------------------------------------------------------------------

void CustomizeContextMenuPopup::onSearchTextChanged(const QString& text) {
  static bool busy = false;
  if (busy) return;
  busy = true;
  m_commandListTree->searchItems(text);
  busy = false;
}

//=============================================================================
// ContextMenuTreeDelegate
//-----------------------------------------------------------------------------

void ContextMenuTreeDelegate::paint(QPainter* painter,
                                    const QStyleOptionViewItem& option,
                                    const QModelIndex& index) const {
  const ContextMenuTree* tree =
      qobject_cast<const ContextMenuTree*>(option.widget);
  if (!tree) return;
  QString text;
  QAction* action = tree->actionFromIndex(index, text);
  if (!action) return;

  // 条件タグの描画
  uint mask = action->data().toUInt();

  QRect textRect = option.rect;

  int condCount        = count_bits(mask);
  int maxCondAreaWidth = 48;

  if (condCount > 0) {
    int posOffset = std::min(
        12,
        (int)std::ceil((double)(maxCondAreaWidth - 12) / (double)condCount));

    QRect condIconRect(option.rect.left(), option.rect.center().y() - 6, 12,
                       12);

    uint curMask = Condition01;
    int c        = 0;
    while (1) {
      if (mask & curMask) {
        painter->drawPixmap(condIconRect,
                            CustomContextMenuManager::instance()->getCondIcon(
                                (CONDITION_MASKS)curMask));
        condIconRect.translate(posOffset, 0);
        c++;
        if (c == condCount) break;
      }
      if (curMask == Condition20) break;
      curMask = curMask << 1;
    }
  }

  textRect.setLeft(textRect.left() +
                   std::min(condCount * 12, maxCondAreaWidth) + 5);
  painter->drawText(textRect, text);
}
