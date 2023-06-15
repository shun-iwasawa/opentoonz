#pragma once

#ifndef VIEWER_PANE_INCLUDED
#define VIEWER_PANE_INCLUDED

#include "sceneviewer.h"
#include "toonzqt/intfield.h"
#include "toonzqt/keyframenavigator.h"
#include "toonzqt/flipconsoleowner.h"
#include "saveloadqsettings.h"

#include <QFrame>

class SceneViewer;
class QPoint;
class QToolBar;
class QLabel;
class QSlider;
class QActionGroup;
class QButtonGroup;
class QToolBar;
class Ruler;

//=============================================================================
// ViewerPanel
//-----------------------------------------------------------------------------

class FlipConsole;
class TXshLevel;

enum VP_Parts {
  VPPARTS_None        = 0,
  VPPARTS_PLAYBAR     = 0x1,
  VPPARTS_FRAMESLIDER = 0x2,
  VPPARTS_TOOLBAR     = 0x4,
  VPPARTS_TOOLOPTIONS = 0x8,
  VPPARTS_End         = 0x10,

  VPPARTS_ALL       = VPPARTS_PLAYBAR | VPPARTS_FRAMESLIDER,
  VPPARTS_COMBO_ALL = VPPARTS_ALL | VPPARTS_TOOLBAR | VPPARTS_TOOLOPTIONS
};

enum VTitleBar_Buttons {
  VTBButtons_None             = 0,
  VTBButtons_SafeArea         = 0x1,
  VTBButtons_FieldGuide       = 0x2,
  VTBButtons_ViewModeCamStand = 0x4,
  VTBButtons_ViewMode3D       = 0x8,
  VTBButtons_ViewModeCamera   = 0x10,
  VTBButtons_Freeze           = 0x20,
  VTBButtons_Preview          = 0x40,
  VTBButtons_SubPreview       = 0x80,
  VTBButtons_Hamburger        = 0x100,
  VTBButtons_All              = 0x200 - 1
};

class BaseViewerPanel : public QFrame,
                        public FlipConsoleOwner,
                        public SaveLoadQSettings {
  Q_OBJECT
protected:
  friend class SceneViewer;
  QVBoxLayout *m_mainLayout;
  SceneViewer *m_sceneViewer;
  ImageUtils::FullScreenWidget *m_fsWidget;
  FlipConsole *m_flipConsole;
  ViewerKeyframeNavigator *m_keyFrameButton;
  TPanelTitleBarButtonSet *m_referenceModeBs;
  TPanelTitleBarButtonForPreview *m_previewButton;
  TPanelTitleBarButtonForPreview *m_subcameraPreviewButton;
  bool m_onionSkinActive = false;
  UINT m_visiblePartsFlag;
  bool m_playSound     = true;
  bool m_hasSoundtrack = false;
  bool m_playing       = false;
  double m_fps;
  int m_viewerFps;
  double m_samplesPerFrame;
  bool m_first         = true;
  TSoundTrack *m_sound = NULL;

  bool m_isActive = false;

  QMap<VTitleBar_Buttons, QWidget *> m_titleBarButtons;
  TPanelTitleBar *m_titleBar;
  UINT m_titleBarMask;

public:
  BaseViewerPanel(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
  ~BaseViewerPanel() {}

  virtual void updateShowHide();
  virtual void addShowHideContextMenu(QMenu *);

  // toggle show/hide of the widgets according to m_visiblePartsFlag
  void setVisiblePartsFlag(UINT flag);

  void onDrawFrame(int frame, const ImagePainter::VisualSettings &settings,
                   QElapsedTimer *timer, qint64 targetInstant) override;
  // return true when the owner is viewer panel
  bool isViewer() override { return true; }

  void onEnterPanel() {
    m_sceneViewer->setFocus(Qt::OtherFocusReason);
    // activate shortcut key for this flipconsole
    m_flipConsole->makeCurrent();
  }
  void onLeavePanel() { m_sceneViewer->clearFocus(); }

  // SaveLoadQSettings
  virtual void save(QSettings &settings) const override;
  virtual void load(QSettings &settings) override;

  void initializeTitleBar(TPanelTitleBar *titleBar);
  void addTitleBarMenuItem(UINT id, const QString &text, QMenu *menu);
  void placeTitleBarButtons();

  void getPreviewButtonStates(bool &prev, bool &subCamPrev);

protected:
  void contextMenuEvent(QContextMenuEvent *event) override;
  void showEvent(QShowEvent *) override;
  void hideEvent(QHideEvent *) override;
  void enableFlipConsoleForCamerastand(bool on);
  void playAudioFrame(int frame);
  bool hasSoundtrack();

  virtual void checkOldVersionVisblePartsFlags(QSettings &settings) = 0;

public slots:

  void changeWindowTitle();
  void updateFrameRange();
  void onSceneChanged();
  void onXshLevelSwitched(TXshLevel *);
  void updateFrameMarkers();
  void onButtonPressed(FlipConsole::EGadget button);
  void setFlipHButtonChecked(bool checked);
  void setFlipVButtonChecked(bool checked);
  void enableFullPreview(bool enabled);
  void enableSubCameraPreview(bool enabled);

protected slots:

  void onFrameSwitched();
  void onSceneSwitched();
  void onFrameTypeChanged();
  void onPlayingStatusChanged(bool playing);
  // for showing/hiding the parts
  void onShowHideActionTriggered(QAction *);
  void onPreviewStatusChanged();
  void onActiveViewerChanged();
  void onCustomizeTitleBarPressed(QAction *);
};

class SceneViewerPanel final : public BaseViewerPanel {
  Q_OBJECT
public:
  SceneViewerPanel(QWidget *parent       = 0,
                   Qt::WindowFlags flags = Qt::WindowFlags());
  ~SceneViewerPanel() {}

protected:
  void checkOldVersionVisblePartsFlags(QSettings &settings) override;
};

#endif
