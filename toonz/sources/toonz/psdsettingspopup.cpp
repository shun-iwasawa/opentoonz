

#include "psdsettingspopup.h"

// Tnz6 includes
#include "tapp.h"

// TnzQt includes
#include "toonzqt/checkbox.h"

// TnzLib includes
#include "toonz/toonzscene.h"
#include "toonz/tscenehandle.h"

// TnzCore includes
#include "tconvert.h"

// Qt includes
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QRadioButton>
#include <QSize>
#include <QStringList>
#include <QTreeWidgetItem>
#include <QMainWindow>
#include <QStackedLayout>

using namespace DVGui;
#define REF_LAYER_BY_NAME

QStringList modesDescription;

// Per adesso non appare
// Costruisce la stringa delle info della psd da caricare che comparirà nel
// popup:
// Path, Dimensioni, numero di livelli, ecc..
static void doPSDInfo(TFilePath psdpath, QTreeWidget *psdTree) {
  psdTree->clear();
  try {
    TPSDReader *psdreader = new TPSDReader(psdpath);

    TPSDHeaderInfo header = psdreader->getPSDHeaderInfo();
    int width             = header.cols;
    int height            = header.rows;
    int depth             = header.depth;
    int channels          = header.channels;
    int layersCount       = header.layersCount;
    QString filename =
        QString::fromStdString(psdpath.getName() + psdpath.getDottedType());
    QString parentDir =
        QString::fromStdWString(psdpath.getParentDir().getWideString());
    QString dimension = QString::number(width) + "x" + QString::number(height);
    QList<QTreeWidgetItem *> items;
    items.append(new QTreeWidgetItem(
        (QTreeWidget *)0, QStringList(QString("Filename: %1").arg(filename))));
    items.append(new QTreeWidgetItem(
        (QTreeWidget *)0,
        QStringList(QString("Parent dir: %1").arg(parentDir))));
    items.append(new QTreeWidgetItem(
        (QTreeWidget *)0,
        QStringList(QString("Dimension: %1").arg(dimension))));
    items.append(new QTreeWidgetItem(
        (QTreeWidget *)0,
        QStringList(QString("Depth: %1").arg(QString::number(depth)))));
    items.append(new QTreeWidgetItem(
        (QTreeWidget *)0,
        QStringList(QString("Channels: %1").arg(QString::number(channels)))));
    QTreeWidgetItem *layersItem = new QTreeWidgetItem((QTreeWidget *)0);
    int count                   = 0;
    QList<QTreeWidgetItem *> layersItemChildren;
    layersItemChildren.append(layersItem);
    int scavenge = 0;
    for (int i = layersCount - 1; i >= 0; i--) {
      TPSDLayerInfo *li = psdreader->getLayerInfo(i);
      int width         = li->right - li->left;
      int height        = li->bottom - li->top;
      QString layerName = li->name;
      if (strcmp(li->name, "</Layer group>") == 0 ||
          strcmp(li->name, "</Layer set>") == 0) {
        scavenge--;
      } else if (li->section > 0 && li->section <= 3) {
        QTreeWidgetItem *child = new QTreeWidgetItem((QTreeWidget *)0);
        child->setText(0, layerName);
        layersItemChildren[scavenge]->addChild(child);
        layersItemChildren.append(child);
        scavenge++;
      } else if (width > 0 && height > 0) {
        if (scavenge >= 0) {
          layersItemChildren[scavenge]->addChild(new QTreeWidgetItem(
              (QTreeWidget *)0, QStringList(QString("%1").arg(layerName))));
          count++;
        }
      }
    }
    QString layerItemText =
        "Layers: " +
        QString::number(count);  //+" ("+QString::number(layersCount)+")";
    layersItem->setText(0, layerItemText);
    items.append(layersItem);

    psdTree->insertTopLevelItems(0, items);
  } catch (TImageException &e) {
    error(QString::fromStdString(::to_string(e.getMessage())));
    return;
  }
}

namespace {
  class TitleTextLabel : public QLabel
  {
  public:
    TitleTextLabel(const QString &text, QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::WindowFlags())
    : QLabel(text, parent, f) {
      setObjectName("TitleTxtLabel");
    }
  };
}

//=============================================================================
// PsdSettingsPopup
//-----------------------------------------------------------------------------

PsdSettingsPopup::PsdSettingsPopup()
    : Dialog(TApp::instance()->getMainWindow(), true, true, "PsdSettings")
    , m_mode(FLAT) {

  setWindowTitle(tr("Load PSD File"));
  if (modesDescription.isEmpty()) {
    modesDescription
      << tr("Flatten visible document layers into a single image. Layer "
        "styles are maintained.")
      << tr("Load document layers as frames into a single xsheet column.")
      << tr("Load document layers as xhseet columns.");
  }
  
  m_filename = new QLabel(tr(""));
  m_parentDir = new QLabel(tr(""));
  m_loadMode = new QComboBox();
  m_modeDescription = new QTextEdit(modesDescription[0]);
  m_createSubXSheet = new CheckBox(tr("Expose in a Sub-xsheet"));
  m_levelNameType = new QComboBox();

  m_skipInvisible = new CheckBox(tr("Skip Invisible Layers"));
  m_skipBackground = new CheckBox(tr("Skip Background Layer"));

  m_filename->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_filename->setFixedHeight(WidgetHeight);
  m_parentDir->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  m_parentDir->setFixedHeight(WidgetHeight);

  m_loadMode->addItem(tr("Single Image"), QString("Single Image"));
  m_loadMode->addItem(tr("Frames"), QString("Frames"));
  m_loadMode->addItem(tr("Columns"), QString("Columns"));
  m_loadMode->setFixedHeight(WidgetHeight);
  m_loadMode->setFixedWidth(120);

  m_modeDescription->setFixedHeight(40);
  m_modeDescription->setMinimumWidth(250);
  m_modeDescription->setReadOnly(true);
  m_createSubXSheet->setMaximumHeight(WidgetHeight);
  m_createSubXSheet->setEnabled(false);

  QStringList types;
  types << tr("FileName#LayerName") << tr("LayerName");
  m_levelNameType->addItems(types);
  m_levelNameType->setFixedHeight(WidgetHeight);
  m_levelNameType->setEnabled(false);

  m_frameGroupOptions = new QComboBox(this);
  m_frameGroupOptions->addItem(tr("Ignore groups"), IgnoreGroups);
  m_frameGroupOptions->addItem(tr("Combine layers in a group as a single image"), GroupAsSingleImage);
  
  m_columnsGroupOptions = new QComboBox(this);
  m_columnsGroupOptions->addItem(tr("Ignore groups"), IgnoreGroups);
  m_columnsGroupOptions->addItem(tr("Expose layers in a group as columns in a sub-xsheet"), GroupAsSubXsheet);
  m_columnsGroupOptions->addItem(tr("Expose layers in a group as frames in a column"), GroupAsColumn);
  
  QHBoxLayout *layinfo = new QHBoxLayout;
  {
    QGridLayout *grid = new QGridLayout();
    grid->setColumnMinimumWidth(0, 65);
    {
      grid->addWidget(new TitleTextLabel(tr("Name:")), 0, 0, Qt::AlignRight);
      grid->addWidget(m_filename, 0, 1, Qt::AlignLeft);
      grid->addWidget(new TitleTextLabel(tr("Path:")), 1, 0, Qt::AlignRight);
      grid->addWidget(m_parentDir, 1, 1, Qt::AlignLeft);
    }
    layinfo->addLayout(grid);
    layinfo->addStretch();
  }
  addLayout(layinfo, false);
  
  addSeparator();
  
  QHBoxLayout *modeLayout = new QHBoxLayout;
  {
    QGridLayout *gridMode = new QGridLayout();
    gridMode->setColumnMinimumWidth(0, 65);
    gridMode->setMargin(0);
    {
      gridMode->addWidget(new TitleTextLabel(tr("Load As:")), 0, 0, Qt::AlignRight);
      gridMode->addWidget(m_loadMode, 0, 1, Qt::AlignLeft);
      gridMode->addWidget(m_modeDescription, 1, 1, Qt::AlignLeft);
      gridMode->addWidget(m_createSubXSheet, 2, 1, Qt::AlignLeft);
      gridMode->addWidget(new TitleTextLabel(tr("Level Name:")), 3, 0, Qt::AlignRight);
      gridMode->addWidget(m_levelNameType, 3, 1, Qt::AlignLeft);
    }
    modeLayout->addLayout(gridMode);
    modeLayout->addStretch();
  }
  addLayout(modeLayout, false);

  m_groupOptionStack = new QStackedLayout();
  m_groupOptionStack->addWidget(new QWidget(this)); //Flat

  QWidget* frameGroup = new QWidget();
  QGridLayout *frameGroupLayout = new QGridLayout();
  frameGroupLayout->setMargin(0);
  frameGroupLayout->setHorizontalSpacing(5);
  frameGroupLayout->setVerticalSpacing(5);
  {
    frameGroupLayout->addWidget(new QLabel(tr("Group Option:"), this), 0, 0, Qt::AlignRight|Qt::AlignVCenter);
    frameGroupLayout->addWidget(m_frameGroupOptions, 0, 1 );

    frameGroupLayout->addWidget(m_skipInvisible, 1, 0, 1, 2);
    frameGroupLayout->addWidget(m_skipBackground, 2, 0, 1, 2);
  }
  frameGroupLayout->setColumnStretch(2, 1);
  frameGroup->setLayout(frameGroupLayout);
  m_groupOptionStack->addWidget(frameGroup); //Frames

  QWidget* columnGroup = new QWidget();
  QHBoxLayout *columnGroupLayout = new QHBoxLayout();
  columnGroupLayout->setMargin(0);
  columnGroupLayout->setSpacing(5);
  {
    columnGroupLayout->addWidget(new QLabel(tr("Group Option:"), this), 0);
    columnGroupLayout->addWidget(m_columnsGroupOptions, 0);
    columnGroupLayout->addStretch(1);
  }
  columnGroup->setLayout(columnGroupLayout);
  m_groupOptionStack->addWidget(columnGroup); //Columns

  addLayout(m_groupOptionStack, false);

  bool ret = true;
  ret = ret && connect(m_loadMode, SIGNAL(currentIndexChanged(const QString &)),
                       SLOT(onModeChanged()));
  
  ret = ret && connect(m_frameGroupOptions, SIGNAL(activated(int)), this,
                       SLOT(onGroupOptionChange()));
  ret = ret && connect(m_columnsGroupOptions, SIGNAL(activated(int)), this,
    SLOT(onGroupOptionChange()));
  assert(ret);

  m_okBtn     = new QPushButton(tr("OK"), this);
  m_cancelBtn = new QPushButton(tr("Cancel"), this);
  connect(m_okBtn, SIGNAL(clicked()), this, SLOT(onOk()));
  connect(m_cancelBtn, SIGNAL(clicked()), this, SLOT(close()));
  addButtonBarWidget(m_okBtn, m_cancelBtn);
}

//-----------------------------------------------------------------------------

void PsdSettingsPopup::setPath(const TFilePath &path) {
  m_path = path;
  // doPSDInfo(path,m_psdTree);
  QString filename =
      QString::fromStdString(path.getName());  //+psdpath.getDottedType());
  QString pathLbl =
      QString::fromStdWString(path.getParentDir().getWideString());
  m_filename->setText(filename);
  m_parentDir->setText(pathLbl);
}

void PsdSettingsPopup::onOk() {
  doPsdParser();
  accept();
}

bool PsdSettingsPopup::subxsheet() {
  return (m_createSubXSheet->isEnabled() && m_createSubXSheet->isChecked());
}

int PsdSettingsPopup::levelNameType() {
  return m_levelNameType->currentIndex();
}

void PsdSettingsPopup::onModeChanged() {
  QString mode = m_loadMode->currentData().toString();
  if (mode == "Single Image") {
    m_mode = FLAT;
    m_modeDescription->setText(modesDescription[0]);
    m_createSubXSheet->setEnabled(false);
    m_levelNameType->setEnabled(false);

    m_groupOptionStack->setCurrentIndex(0);
  } else if (mode == "Frames") {
    if (m_frameGroupOptions->currentData() == IgnoreGroups)
      m_mode = FRAMES;
    else
      m_mode = FRAMES_GROUP;

    m_modeDescription->setText(modesDescription[1]);
    m_createSubXSheet->setEnabled(false);
    m_levelNameType->setEnabled(false);

    m_groupOptionStack->setCurrentIndex(1);
  } else if (mode == "Columns") {
    if (m_columnsGroupOptions->currentData() == GroupAsSubXsheet ||
        m_columnsGroupOptions->currentData() == GroupAsColumn)
      m_mode = FOLDER;
    else
      m_mode = COLUMNS;

    m_modeDescription->setText(modesDescription[2]);
    m_createSubXSheet->setEnabled(true);
    m_levelNameType->setEnabled(true);

    m_groupOptionStack->setCurrentIndex(2);
  } else {
    assert(false);
  }
}

void PsdSettingsPopup::onGroupOptionChange() {
  QString mode = m_loadMode->currentData().toString();
  if (mode == "Frames") {
    if (m_frameGroupOptions->currentData() == IgnoreGroups)
      m_mode = FRAMES;
    else
      m_mode = FRAMES_GROUP;
  }
  else if (mode == "Columns") {
    if (m_columnsGroupOptions->currentData() == GroupAsSubXsheet ||
      m_columnsGroupOptions->currentData() == GroupAsColumn)
      m_mode = FOLDER;
    else
      m_mode = COLUMNS;
  }
  else 
    assert(false);
}

int PsdSettingsPopup::getGroupOption() {
  QString mode = m_loadMode->currentData().toString();
  if (mode == "Frames")  return m_frameGroupOptions->currentData().toInt();
  else if (mode == "Columns") return m_columnsGroupOptions->currentData().toInt();
  else return -1;
}

void PsdSettingsPopup::doPsdParser() {
  TFilePath psdpath =
      TApp::instance()->getCurrentScene()->getScene()->decodeFilePath(m_path);
  std::string mode = "";
  switch (m_mode) {
  case FLAT: {
    break;
  }
  case FRAMES: {
    mode = "#frames";
    if (m_skipInvisible->isChecked())
      mode += "#SI";
    if (m_skipBackground->isChecked())
      mode += "#SB";
    std::string name =
        psdpath.getName() + "#1" + mode + psdpath.getDottedType();
    psdpath = psdpath.getParentDir() + TFilePath(name);
    break;
  }
  case FRAMES_GROUP: {
    mode = "#framesgroup";
    if (m_skipInvisible->isChecked())
      mode += "#SI";
    if (m_skipBackground->isChecked())
      mode += "#SB";
    std::string name =
      psdpath.getName() + "#1" + mode + psdpath.getDottedType();
    psdpath = psdpath.getParentDir() + TFilePath(name);
    break;
  }
  case COLUMNS: {
    std::string name = psdpath.getName() + "#1" + psdpath.getDottedType();
    psdpath          = psdpath.getParentDir() + TFilePath(name);
    break;
  }
  case FOLDER: {
    mode = "#group";
    std::string name =
        psdpath.getName() + "#1" + mode + psdpath.getDottedType();
    psdpath = psdpath.getParentDir() + TFilePath(name);
    break;
  }
  default: {
    assert(false);
    return;
  }
  }
  try {
    m_psdparser = new TPSDParser(psdpath);
    m_psdLevelPaths.clear();
    for (int i = 0; i < m_psdparser->getLevelsCount(); i++) {
      int layerId      = m_psdparser->getLevelId(i);
      std::string name = m_path.getName();
      if (layerId > 0 && (m_mode != FRAMES && m_mode != FRAMES_GROUP)) {
        if (m_levelNameType->currentIndex() == 0)  // FileName#LevelName
          name += "#" + std::to_string(layerId);
        else  // LevelName
          name += "##" + std::to_string(layerId);
      }
      if (mode != "") name += mode;
      name += m_path.getDottedType();
      TFilePath psdpath = m_path.getParentDir() + TFilePath(name);
      m_psdLevelPaths.push_back(psdpath);
    }
  } catch (TImageException &e) {
    error(QString::fromStdString(::to_string(e.getMessage())));
    return;
  }
}
TFilePath PsdSettingsPopup::getPsdPath(int levelIndex) {
  assert(levelIndex >= 0 && levelIndex < m_psdLevelPaths.size());
  return m_psdLevelPaths[levelIndex];
}
TFilePath PsdSettingsPopup::getPsdFramePath(int levelIndex, int frameIndex) {
  int layerId      = m_psdparser->getLevelId(levelIndex);
  int frameId      = m_psdparser->getFrameLayerIds(layerId, frameIndex)[0];
  std::string name = m_path.getName();
  if (frameId > 0) name += "#" + std::to_string(frameId);
  name += m_path.getDottedType();
  TFilePath psdpath = TApp::instance()
                          ->getCurrentScene()
                          ->getScene()
                          ->decodeFilePath(m_path)
                          .getParentDir() +
                      TFilePath(name);
  return psdpath;
}
int PsdSettingsPopup::getFramesCount(int levelIndex) {
  // assert(levelIndex>=0 && levelIndex<m_levels.size());
  // return m_levels[levelIndex].framesCount;
  int levelId = m_psdparser->getLevelId(levelIndex);
  return m_psdparser->getFramesCount(levelId);
}
bool PsdSettingsPopup::isFolder(int levelIndex) {
  // assert(levelIndex>=0 && levelIndex<m_levels.size());
  // return m_levels[levelIndex].isFolder;
  return m_psdparser->isFolder(levelIndex);
}
bool PsdSettingsPopup::isSubFolder(int levelIndex, int frameIndex) {
  return m_psdparser->isSubFolder(levelIndex, frameIndex);
}
int PsdSettingsPopup::getSubfolderLevelIndex(int psdLevelIndex,
                                             int frameIndex) {
  int levelId        = m_psdparser->getLevelId(psdLevelIndex);
  int frameId        = m_psdparser->getFrameLayerIds(levelId, frameIndex)[0];
  int subFolderIndex = m_psdparser->getLevelIndexById(frameId);
  return subFolderIndex;
}

//-----------------------------------------------------------------------------

//=============================================================================

// OpenPopupCommandHandler<PsdSettingsPopup>
// openPsdSettingsPopup(MI_SceneSettings);
