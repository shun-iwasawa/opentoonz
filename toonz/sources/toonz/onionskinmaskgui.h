#pragma once

#ifndef ONIONSKINMASKGUI
#define ONIONSKINMASKGUI

#include <QObject>
#include "customcontextmenumanager.h"

class OnionSkinMask;
class QMenu;

//=============================================================================
namespace OnioniSkinMaskGUI {
//-----------------------------------------------------------------------------

// usd in xsheet row area
void registerOnionSkinCommand(
    QMenu*, QMap<QString, QString>& commandLabels,
    QMap<CONDITION_MASKS, QString>& conditionDescriptions);
QAction* doCustomContextAction(const QString&);
void setContextMenuConditions(uint& mask);

// Da fare per la filmstrip!!
void addOnionSkinCommand(QMenu*, bool isFilmStrip = false);

void resetShiftTraceFrameOffset();

//=============================================================================
// OnionSkinSwitcher
//-----------------------------------------------------------------------------

class OnionSkinSwitcher final : public QObject {
  Q_OBJECT
  QMap<QString, QAction*> m_osActions;
  OnionSkinSwitcher();

public:
  static OnionSkinSwitcher* instance();

  OnionSkinMask getMask() const;

  void setMask(const OnionSkinMask& mask);

  bool isActive() const;
  bool isWholeScene() const;
  QAction* getActionFromId(const QString&);

public slots:
  void activate();
  void deactivate();
  void setWholeScene();
  void setSingleLevel();
  void clearFOS();
  void clearMOS();
  void clearOS();
};

//-----------------------------------------------------------------------------
}  // namespace OnioniSkinMaskGUI
//-----------------------------------------------------------------------------

#endif  // ONIONSKINMASKGUI
