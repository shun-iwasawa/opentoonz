

#include "onionskinmaskgui.h"
#include "tapp.h"
#include "toonz/tonionskinmaskhandle.h"
#include "toonz/tframehandle.h"
#include "toonz/txshlevelhandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/tcolumnhandle.h"
#include "toonz/txshsimplelevel.h"

#include "toonz/onionskinmask.h"

#include <QMenu>

//=============================================================================
// OnioniSkinMaskGUI::OnionSkinSwitcher
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
namespace OnioniSkinMaskGUI {
const QString CMenu_DeactivateOnionSkin      = "CMenu_DeactivateOnionSkin";
const QString CMenu_LimitOnionSkinToLevel    = "CMenu_LimitOnionSkinToLevel";
const QString CMenu_ExtendOnionSkinToScene   = "CMenu_ExtendOnionSkinToScene";
const QString CMenu_ClearAllOnionSkinMarkers = "CMenu_ClearAllOnionSkinMarkers";
const QString CMenu_ClearAllFixedOnionSkinMarkers =
    "CMenu_ClearAllFixedOnionSkinMarkers";
const QString CMenu_ClearAllRelativeOnionSkinMarkers =
    "CMenu_ClearAllRelativeOnionSkinMarkers";
const QString CMenu_ActivateOnionSkin = "CMenu_ActivateOnionSkin";
};  // namespace OnioniSkinMaskGUI

OnioniSkinMaskGUI::OnionSkinSwitcher::OnionSkinSwitcher() {
  m_osActions = {
      {CMenu_DeactivateOnionSkin,
       new QAction(QObject::tr("Deactivate Onion Skin"))},
      {CMenu_LimitOnionSkinToLevel,
       new QAction(QObject::tr("Limit Onion Skin To Level"))},
      {CMenu_ExtendOnionSkinToScene,
       new QAction(QObject::tr("Extend Onion Skin To Scene"))},
      {CMenu_ClearAllOnionSkinMarkers,
       new QAction(QObject::tr("Clear All Onion Skin Markers"))},
      {CMenu_ClearAllFixedOnionSkinMarkers,
       new QAction(QObject::tr("Clear All Fixed Onion Skin Markers"))},
      {CMenu_ClearAllRelativeOnionSkinMarkers,
       new QAction(QObject::tr("Clear All Relative Onion Skin Markers"))},
      {CMenu_ActivateOnionSkin,
       new QAction(QObject::tr("Activate Onion Skin"))}};

  bool ret = true;
  ret      = ret && connect(m_osActions[CMenu_DeactivateOnionSkin],
                            SIGNAL(triggered()), this, SLOT(deactivate()));
  ret      = ret && connect(m_osActions[CMenu_LimitOnionSkinToLevel],
                            SIGNAL(triggered()), this, SLOT(setSingleLevel()));
  ret      = ret && connect(m_osActions[CMenu_ExtendOnionSkinToScene],
                            SIGNAL(triggered()), this, SLOT(setWholeScene()));
  ret      = ret && connect(m_osActions[CMenu_ClearAllOnionSkinMarkers],
                            SIGNAL(triggered()), this, SLOT(clearOS()));
  ret      = ret && connect(m_osActions[CMenu_ClearAllFixedOnionSkinMarkers],
                            SIGNAL(triggered()), this, SLOT(clearFOS()));
  ret      = ret && connect(m_osActions[CMenu_ClearAllRelativeOnionSkinMarkers],
                            SIGNAL(triggered()), this, SLOT(clearMOS()));
  ret      = ret && connect(m_osActions[CMenu_ActivateOnionSkin],
                            SIGNAL(triggered()), this, SLOT(activate()));
  assert(ret);
}

OnioniSkinMaskGUI::OnionSkinSwitcher *
OnioniSkinMaskGUI::OnionSkinSwitcher::instance() {
  static OnionSkinSwitcher _instance;
  return &_instance;
}

OnionSkinMask OnioniSkinMaskGUI::OnionSkinSwitcher::getMask() const {
  return TApp::instance()->getCurrentOnionSkin()->getOnionSkinMask();
}

//------------------------------------------------------------------------------

void OnioniSkinMaskGUI::OnionSkinSwitcher::setMask(const OnionSkinMask &mask) {
  TApp::instance()->getCurrentOnionSkin()->setOnionSkinMask(mask);
  TApp::instance()->getCurrentOnionSkin()->notifyOnionSkinMaskChanged();
}

//------------------------------------------------------------------------------

bool OnioniSkinMaskGUI::OnionSkinSwitcher::isActive() const {
  OnionSkinMask osm = getMask();
  return osm.isEnabled() && !osm.isEmpty();
}

//------------------------------------------------------------------------------

bool OnioniSkinMaskGUI::OnionSkinSwitcher::isWholeScene() const {
  OnionSkinMask osm = getMask();
  return osm.isWholeScene();
}

//------------------------------------------------------------------------------

void OnioniSkinMaskGUI::OnionSkinSwitcher::activate() {
  OnionSkinMask osm = getMask();
  if (osm.isEnabled() && !osm.isEmpty()) return;
  if (osm.isEmpty()) {
    osm.setMos(-1, true);
    osm.setMos(-2, true);
    osm.setMos(-3, true);
  }
  osm.enable(true);
  setMask(osm);
}

//------------------------------------------------------------------------------

void OnioniSkinMaskGUI::OnionSkinSwitcher::deactivate() {
  OnionSkinMask osm = getMask();
  if (!osm.isEnabled() || osm.isEmpty()) return;
  osm.enable(false);
  setMask(osm);
}

//------------------------------------------------------------------------------

void OnioniSkinMaskGUI::OnionSkinSwitcher::setWholeScene() {
  OnionSkinMask osm = getMask();
  osm.setIsWholeScene(true);
  setMask(osm);
}

//------------------------------------------------------------------------------

void OnioniSkinMaskGUI::OnionSkinSwitcher::setSingleLevel() {
  OnionSkinMask osm = getMask();
  osm.setIsWholeScene(false);
  setMask(osm);
}

void OnioniSkinMaskGUI::OnionSkinSwitcher::clearFOS() {
  OnionSkinMask osm = getMask();

  for (int i = (osm.getFosCount() - 1); i >= 0; i--)
    osm.setFos(osm.getFos(i), false);

  setMask(osm);
}

void OnioniSkinMaskGUI::OnionSkinSwitcher::clearMOS() {
  OnionSkinMask osm = getMask();

  for (int i = (osm.getMosCount() - 1); i >= 0; i--)
    osm.setMos(osm.getMos(i), false);

  setMask(osm);
}

void OnioniSkinMaskGUI::OnionSkinSwitcher::clearOS() {
  clearFOS();
  clearMOS();
}

QAction *OnioniSkinMaskGUI::OnionSkinSwitcher::getActionFromId(
    const QString &id) {
  return m_osActions.value(id, nullptr);
}

//------------------------------------------------------------------------------
void OnioniSkinMaskGUI::registerOnionSkinCommand(
    QMenu *menu, QMap<QString, QString> &commandLabels,
    QMap<CONDITION_MASKS, QString> &conditionDescriptions) {
  // Condition starts from 02 (see RowArea::registerContextMenus)
  conditionDescriptions[Condition02] =
      QObject::tr("Onion skin is activated.", "context menu condition");
  conditionDescriptions[Condition03] =
      QObject::tr("Onion skin is deactivated.", "context menu condition");
  conditionDescriptions[Condition04] = QObject::tr(
      "Onion skin is extended to whole scene.", "context menu condition");
  conditionDescriptions[Condition05] = QObject::tr(
      "Onion skin is limited to single level.", "context menu condition");
  conditionDescriptions[Condition06] =
      QObject::tr("Onion skin has relative or fixed or both markers.",
                  "context menu condition");
  conditionDescriptions[Condition07] =
      QObject::tr("Onion skin has both relative and fixed markers.",
                  "context menu condition");

  // Conditions02 : if (switcher.isActive())
  {
    commandLabels.insert(CMenu_DeactivateOnionSkin,
                         QObject::tr("Deactivate Onion Skin"));
    menu->addAction(CMenu_DeactivateOnionSkin)
        ->setData(Condition01 | Condition02);

    // Conditions04 : if (switcher.isWholeScene())
    {
      commandLabels.insert(CMenu_LimitOnionSkinToLevel,
                           QObject::tr("Limit Onion Skin To Level"));
      menu->addAction(CMenu_LimitOnionSkinToLevel)
          ->setData(Condition01 | Condition02 | Condition04);
    }
    // Conditions05 : !switcher.isWholeScene()
    {
      commandLabels.insert(CMenu_ExtendOnionSkinToScene,
                           QObject::tr("Extend Onion Skin To Scene"));
      menu->addAction(CMenu_ExtendOnionSkinToScene)
          ->setData(Condition01 | Condition02 | Condition05);
    }
    OnionSkinMask osm = OnionSkinSwitcher::instance()->getMask();
    // Conditions06 : if (osm.getFosCount() || osm.getMosCount())
    {
      commandLabels.insert(CMenu_ClearAllOnionSkinMarkers,
                           QObject::tr("Clear All Onion Skin Markers"));
      menu->addAction(CMenu_ClearAllOnionSkinMarkers)
          ->setData(Condition01 | Condition02 | Condition06);
    }
    // Conditions07 : if (osm.getFosCount() && osm.getMosCount())
    {
      commandLabels.insert(CMenu_ClearAllFixedOnionSkinMarkers,
                           QObject::tr("Clear All Fixed Onion Skin Markers"));
      menu->addAction(CMenu_ClearAllFixedOnionSkinMarkers)
          ->setData(Condition01 | Condition02 | Condition07);
      commandLabels.insert(
          CMenu_ClearAllRelativeOnionSkinMarkers,
          QObject::tr("Clear All Relative Onion Skin Markers"));
      menu->addAction(CMenu_ClearAllRelativeOnionSkinMarkers)
          ->setData(Condition01 | Condition02 | Condition07);
    }
  }
  // Conditions03 : !switcher.isActive()
  {
    commandLabels.insert(CMenu_ActivateOnionSkin,
                         QObject::tr("Activate Onion Skin"));
    menu->addAction(CMenu_ActivateOnionSkin)
        ->setData(Condition01 | Condition03);
  }
}
//------------------------------------------------------------------------------

QAction *OnioniSkinMaskGUI::doCustomContextAction(const QString &cmdId) {
  QAction *action = nullptr;
  return OnionSkinSwitcher::instance()->getActionFromId(cmdId);
}

//------------------------------------------------------------------------------

void OnioniSkinMaskGUI::setContextMenuConditions(uint &mask) {
  if (OnionSkinSwitcher::instance()->isActive()) {
    mask |= Condition02;
    if (OnionSkinSwitcher::instance()->isWholeScene())
      mask |= Condition04;
    else
      mask |= Condition05;

    OnionSkinMask osm = OnionSkinSwitcher::instance()->getMask();
    if (osm.getFosCount() || osm.getMosCount()) mask |= Condition06;
    if (osm.getFosCount() && osm.getMosCount()) mask |= Condition07;
  } else
    mask |= Condition03;
}
//------------------------------------------------------------------------------
void OnioniSkinMaskGUI::addOnionSkinCommand(QMenu *menu, bool isFilmStrip) {
  OnioniSkinMaskGUI::OnionSkinSwitcher *switcher =
      OnionSkinSwitcher::instance();
  if (switcher->isActive()) {
    QAction *dectivateOnionSkin =
        menu->addAction(QString(QObject::tr("Deactivate Onion Skin")));
    menu->connect(dectivateOnionSkin, SIGNAL(triggered()), switcher,
                  SLOT(deactivate()));
    if (!isFilmStrip) {
      if (switcher->isWholeScene()) {
        QAction *limitOnionSkinToLevel =
            menu->addAction(QString(QObject::tr("Limit Onion Skin To Level")));
        menu->connect(limitOnionSkinToLevel, SIGNAL(triggered()), switcher,
                      SLOT(setSingleLevel()));
      } else {
        QAction *extendOnionSkinToScene =
            menu->addAction(QString(QObject::tr("Extend Onion Skin To Scene")));
        menu->connect(extendOnionSkinToScene, SIGNAL(triggered()), switcher,
                      SLOT(setWholeScene()));
      }
      OnionSkinMask osm = switcher->getMask();
      if (osm.getFosCount() || osm.getMosCount()) {
        QAction *clearAllOnionSkins = menu->addAction(
            QString(QObject::tr("Clear All Onion Skin Markers")));
        menu->connect(clearAllOnionSkins, SIGNAL(triggered()), switcher,
                      SLOT(clearOS()));
      }
      if (osm.getFosCount() && osm.getMosCount()) {
        QAction *clearFixedOnionSkins = menu->addAction(
            QString(QObject::tr("Clear All Fixed Onion Skin Markers")));
        menu->connect(clearFixedOnionSkins, SIGNAL(triggered()), switcher,
                      SLOT(clearFOS()));
        QAction *clearRelativeOnionSkins = menu->addAction(
            QString(QObject::tr("Clear All Relative Onion Skin Markers")));
        menu->connect(clearRelativeOnionSkins, SIGNAL(triggered()), switcher,
                      SLOT(clearMOS()));
      }
    }
  } else {
    QAction *activateOnionSkin =
        menu->addAction(QString(QObject::tr("Activate Onion Skin")));
    menu->connect(activateOnionSkin, SIGNAL(triggered()), switcher,
                  SLOT(activate()));
  }
}

//------------------------------------------------------------------------------

void OnioniSkinMaskGUI::resetShiftTraceFrameOffset() {
  auto setGhostOffset = [](int firstOffset, int secondOffset) {
    OnionSkinMask osm =
        TApp::instance()->getCurrentOnionSkin()->getOnionSkinMask();
    osm.setShiftTraceGhostFrameOffset(0, firstOffset);
    osm.setShiftTraceGhostFrameOffset(1, secondOffset);
    TApp::instance()->getCurrentOnionSkin()->setOnionSkinMask(osm);
  };

  TApp *app = TApp::instance();
  if (app->getCurrentFrame()->isEditingLevel()) {
    TXshSimpleLevel *level = app->getCurrentLevel()->getSimpleLevel();
    if (!level) {
      setGhostOffset(0, 0);
      return;
    }
    TFrameId fid     = app->getCurrentFrame()->getFid();
    int firstOffset  = (fid > level->getFirstFid()) ? -1 : 0;
    int secondOffset = (fid < level->getLastFid()) ? 1 : 0;

    setGhostOffset(firstOffset, secondOffset);
  } else {  // when scene frame is selected
    TXsheet *xsh       = app->getCurrentXsheet()->getXsheet();
    int col            = app->getCurrentColumn()->getColumnIndex();
    TXshColumn *column = xsh->getColumn(col);
    if (!column || column->isEmpty()) {
      setGhostOffset(0, 0);
      return;
    }
    int r0, r1;
    column->getRange(r0, r1);
    int row         = app->getCurrentFrame()->getFrame();
    TXshCell cell   = xsh->getCell(row, col);
    int firstOffset = -1;
    while (1) {
      int r = row + firstOffset;
      if (r < r0) {
        firstOffset = 0;
        break;
      }
      if (!xsh->getCell(r, col).isEmpty() && xsh->getCell(r, col) != cell) {
        break;
      }
      firstOffset--;
    }
    int secondOffset = 1;
    while (1) {
      int r = row + secondOffset;
      if (r > r1) {
        secondOffset = 0;
        break;
      }
      if (!xsh->getCell(r, col).isEmpty() && xsh->getCell(r, col) != cell) {
        break;
      }
      secondOffset++;
    }
    setGhostOffset(firstOffset, secondOffset);
  }
}
