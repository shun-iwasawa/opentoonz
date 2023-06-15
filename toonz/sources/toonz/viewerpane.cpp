

// TnzCore includes
#include "tconvert.h"
#include "tgeometry.h"
#include "tgl.h"
#include "trop.h"
#include "tstopwatch.h"

// TnzLib includes
#include "toonz/tscenehandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/tframehandle.h"
#include "toonz/tcolumnhandle.h"
#include "toonz/txshlevelhandle.h"
#include "toonz/toonzscene.h"
#include "toonz/sceneproperties.h"
#include "toonz/txsheet.h"
#include "toonz/stage.h"
#include "toonz/stage2.h"
#include "toonz/txshlevel.h"
#include "toonz/txshcell.h"
#include "toonz/tcamera.h"
#include "toonz/tstageobjecttree.h"
#include "toonz/tobjecthandle.h"
#include "toonz/tpalettehandle.h"
#include "toonz/tonionskinmaskhandle.h"
#include "toutputproperties.h"
#include "toonz/preferences.h"
#include "toonz/tproject.h"

// TnzQt includes
#include "toonzqt/menubarcommand.h"
#include "toonzqt/dvdialog.h"
#include "toonzqt/gutil.h"
#include "toonzqt/imageutils.h"

// TnzTools includes
#include "tools/toolhandle.h"

// Tnz6 includes
#include "tapp.h"
#include "mainwindow.h"
#include "sceneviewer.h"
#include "xsheetdragtool.h"
#include "ruler.h"
#include "menubarcommandids.h"
#include "tenv.h"
#include "cellselection.h"

// Qt includes
#include <QPainter>
#include <QVBoxLayout>
#include <QAction>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QRadioButton>
#include <QSlider>
#include <QButtonGroup>
#include <QMenu>
#include <QToolBar>
#include <QMainWindow>
#include <QSettings>
#include <QToolButton>

#include "viewerpane.h"

using namespace DVGui;

extern TEnv::IntVar EnvViewerPreviewBehavior;

// this enum is to keep comaptibility with older versions
enum OldV_Parts {
  OldVPARTS_None        = 0,
  OldVPARTS_PLAYBAR     = 0x1,
  OldVPARTS_FRAMESLIDER = 0x4,
  OldVPARTS_End         = 0x8,
  OldVPARTS_ALL         = OldVPARTS_PLAYBAR | OldVPARTS_FRAMESLIDER
};

//=============================================================================
//
// BaseViewerPanel
//
//-----------------------------------------------------------------------------

BaseViewerPanel::BaseViewerPanel(QWidget *parent, Qt::WindowFlags flags)
    : QFrame(parent), m_titleBarMask(VTBButtons_All) {
  TApp *app = TApp::instance();

  setFrameStyle(QFrame::StyledPanel);

  m_mainLayout = new QVBoxLayout();
  m_mainLayout->setMargin(0);
  m_mainLayout->setSpacing(0);

  // Viewer
  m_fsWidget = new ImageUtils::FullScreenWidget(this);
  m_fsWidget->setWidget(m_sceneViewer = new SceneViewer(m_fsWidget));
  m_sceneViewer->setIsStyleShortcutSwitchable();

  m_keyFrameButton = new ViewerKeyframeNavigator(0, app->getCurrentFrame());
  m_keyFrameButton->setObjectHandle(app->getCurrentObject());
  m_keyFrameButton->setXsheetHandle(app->getCurrentXsheet());

  std::vector<int> buttonMask = {
      FlipConsole::eFilledRaster, FlipConsole::eDefineLoadBox,
      FlipConsole::eUseLoadBox,   FlipConsole::eDecreaseGain,
      FlipConsole::eIncreaseGain, FlipConsole::eResetGain};

  m_flipConsole =
      new FlipConsole(m_mainLayout, buttonMask, false, m_keyFrameButton,
                      "SceneViewerConsole", this, true);

  m_flipConsole->enableButton(FlipConsole::eMatte, false, false);
  m_flipConsole->enableButton(FlipConsole::eSave, false, false);
  m_flipConsole->enableButton(FlipConsole::eCompare, false, false);
  m_flipConsole->enableButton(FlipConsole::eSaveImg, false, false);
  m_flipConsole->enableButton(FlipConsole::eGRed, false, false);
  m_flipConsole->enableButton(FlipConsole::eGGreen, false, false);
  m_flipConsole->enableButton(FlipConsole::eGBlue, false, false);
  m_flipConsole->enableButton(FlipConsole::eBlackBg, false, false);
  m_flipConsole->enableButton(FlipConsole::eWhiteBg, false, false);
  m_flipConsole->enableButton(FlipConsole::eCheckBg, false, false);
  m_flipConsole->setChecked(FlipConsole::eSound, true);
  m_playSound = m_flipConsole->isChecked(FlipConsole::eSound);

  m_flipConsole->setFrameRate(app->getCurrentScene()
                                  ->getScene()
                                  ->getProperties()
                                  ->getOutputProperties()
                                  ->getFrameRate());
  m_flipConsole->setFrameHandle(TApp::instance()->getCurrentFrame());

  bool ret = true;
  // When zoom changed, only if the viewer is active, change window titl
  ret = ret && connect(m_sceneViewer, SIGNAL(onZoomChanged()),
                       SLOT(changeWindowTitle()));
  ret = ret &&
        connect(m_flipConsole, SIGNAL(playStateChanged(bool)),
                TApp::instance()->getCurrentFrame(), SLOT(setPlaying(bool)));
  ret = ret && connect(m_flipConsole, SIGNAL(playStateChanged(bool)), this,
                       SLOT(onPlayingStatusChanged(bool)));
  ret = ret &&
        connect(m_flipConsole, SIGNAL(buttonPressed(FlipConsole::EGadget)),
                m_sceneViewer, SLOT(onButtonPressed(FlipConsole::EGadget)));
  ret =
      ret && connect(m_flipConsole, SIGNAL(buttonPressed(FlipConsole::EGadget)),
                     this, SLOT(onButtonPressed(FlipConsole::EGadget)));

  ret = ret && connect(m_sceneViewer, SIGNAL(previewStatusChanged()), this,
                       SLOT(onPreviewStatusChanged()));
  ret = ret && connect(m_sceneViewer, SIGNAL(onFlipHChanged(bool)), this,
                       SLOT(setFlipHButtonChecked(bool)));
  ret = ret && connect(m_sceneViewer, SIGNAL(onFlipVChanged(bool)), this,
                       SLOT(setFlipVButtonChecked(bool)));

  ret = ret && connect(app->getCurrentScene(), SIGNAL(sceneSwitched()), this,
                       SLOT(onSceneSwitched()));

  ret = ret && connect(app, SIGNAL(activeViewerChanged()), this,
                       SLOT(onActiveViewerChanged()));

  assert(ret);

  setFocusProxy(m_sceneViewer);
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
/*! toggle show/hide of the widgets according to m_visibleFlag
 */

void BaseViewerPanel::updateShowHide() {
  // flip console
  m_flipConsole->showHidePlaybar(m_visiblePartsFlag & VPPARTS_PLAYBAR);
  m_flipConsole->showHideFrameSlider(m_visiblePartsFlag & VPPARTS_FRAMESLIDER);
  update();
}

//-----------------------------------------------------------------------------
/*! showing the show/hide commands
 */

void BaseViewerPanel::contextMenuEvent(QContextMenuEvent *event) {
  QMenu *menu = new QMenu(this);
  addShowHideContextMenu(menu);
  menu->exec(event->globalPos());
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::addShowHideContextMenu(QMenu *menu) {
  QMenu *showHideMenu = menu->addMenu(tr("GUI Show / Hide"));

  // actions
  QAction *playbarSHAct     = showHideMenu->addAction(tr("Playback Toolbar"));
  QAction *frameSliderSHAct = showHideMenu->addAction(tr("Frame Slider"));

  playbarSHAct->setCheckable(true);
  playbarSHAct->setChecked(m_visiblePartsFlag & VPPARTS_PLAYBAR);
  playbarSHAct->setData((UINT)VPPARTS_PLAYBAR);

  frameSliderSHAct->setCheckable(true);
  frameSliderSHAct->setChecked(m_visiblePartsFlag & VPPARTS_FRAMESLIDER);
  frameSliderSHAct->setData((UINT)VPPARTS_FRAMESLIDER);

  QActionGroup *showHideActGroup = new QActionGroup(this);
  showHideActGroup->setExclusive(false);
  showHideActGroup->addAction(playbarSHAct);
  showHideActGroup->addAction(frameSliderSHAct);

  connect(showHideActGroup, SIGNAL(triggered(QAction *)), this,
          SLOT(onShowHideActionTriggered(QAction *)));

  showHideMenu->addSeparator();
  showHideMenu->addAction(CommandManager::instance()->getAction(MI_ViewCamera));
  showHideMenu->addAction(CommandManager::instance()->getAction(MI_ViewTable));
  showHideMenu->addAction(CommandManager::instance()->getAction(MI_FieldGuide));
  showHideMenu->addAction(CommandManager::instance()->getAction(MI_SafeArea));
  showHideMenu->addAction(CommandManager::instance()->getAction(MI_ViewBBox));
  showHideMenu->addAction(
      CommandManager::instance()->getAction(MI_ViewColorcard));
  showHideMenu->addAction(CommandManager::instance()->getAction(MI_ViewRuler));
}

//-----------------------------------------------------------------------------
/*! slot function for show/hide the parts
 */

void BaseViewerPanel::onShowHideActionTriggered(QAction *act) {
  VP_Parts part = (VP_Parts)act->data().toUInt();
  assert(part < VPPARTS_End);

  m_visiblePartsFlag ^= part;

  updateShowHide();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::onCustomizeTitleBarPressed(QAction *act) {
  VTitleBar_Buttons buttonId = (VTitleBar_Buttons)act->data().toUInt();
  assert(buttonId < VTBButtons_Hamburger);

  m_titleBarMask ^= buttonId;

  placeTitleBarButtons();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::onDrawFrame(int frame,
                                  const ImagePainter::VisualSettings &settings,
                                  QElapsedTimer *, qint64) {
  TApp *app = TApp::instance();
  m_sceneViewer->setVisual(settings);

  TFrameHandle *frameHandle = app->getCurrentFrame();

  if (m_sceneViewer->isPreviewEnabled()) {
    class Previewer *pr = Previewer::instance(m_sceneViewer->getPreviewMode() ==
                                              SceneViewer::SUBCAMERA_PREVIEW);
    pr->getRaster(frame - 1, settings.m_recomputeIfNeeded);  // the 'getRaster'
                                                             // starts the
                                                             // render of the
                                                             // frame is not
                                                             // already started
    int curFrame = frame;
    if (frameHandle->isPlaying() &&
        !pr->isFrameReady(
            frame - 1))  // stops on last rendered frame until current is ready!
    {
      while (frame > 0 && !pr->isFrameReady(frame - 1)) frame--;
      if (frame == 0)
        frame = curFrame;  // if no frame is ready, I stay on current...no use
                           // to rewind
      m_flipConsole->setCurrentFrame(frame);
    }
  }

  // assert(frame >= 0); // frame can be negative in rare cases
  if (frame != frameHandle->getFrameIndex() + 1 && !settings.m_drawBlankFrame) {
    int oldFrame = frameHandle->getFrame();
    frameHandle->setCurrentFrame(frame);
    if (!frameHandle->isPlaying() && !frameHandle->isEditingLevel() &&
        oldFrame != frameHandle->getFrame())
      frameHandle->scrubXsheet(
          frame - 1, frame - 1,
          TApp::instance()->getCurrentXsheet()->getXsheet());
  }

  else if (settings.m_blankColor != TPixel::Transparent)
    m_sceneViewer->update();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::showEvent(QShowEvent *event) {
  TApp *app                    = TApp::instance();
  TFrameHandle *frameHandle    = app->getCurrentFrame();
  TSceneHandle *sceneHandle    = app->getCurrentScene();
  TXshLevelHandle *levelHandle = app->getCurrentLevel();
  TXsheetHandle *xshHandle     = app->getCurrentXsheet();

  bool ret = true;

  /*!
  onSceneChanged(): called when the scene changed
  - set new scene's FPS
  - update the range of frame slider with a new framehandle
  - set the marker
  - update key frames
  */
  ret = ret && connect(xshHandle, SIGNAL(xsheetChanged()), this,
                       SLOT(onSceneChanged()));
  ret = ret && connect(sceneHandle, SIGNAL(sceneSwitched()), this,
                       SLOT(onSceneChanged()));
  ret = ret && connect(sceneHandle, SIGNAL(sceneChanged()), this,
                       SLOT(onSceneChanged()));

  /*!
  changeWindowTitle(): called when the scene / level / frame is changed
  - chenge the title text
  */
  ret = ret && connect(sceneHandle, SIGNAL(nameSceneChanged()), this,
                       SLOT(changeWindowTitle()));
  ret = ret && connect(levelHandle, SIGNAL(xshLevelChanged()), this,
                       SLOT(changeWindowTitle()));
  ret = ret && connect(levelHandle, SIGNAL(xshLevelTitleChanged()), this,
                       SLOT(changeWindowTitle()));
  ret = ret && connect(frameHandle, SIGNAL(frameSwitched()), this,
                       SLOT(changeWindowTitle()));

  // updateFrameRange(): update the frame slider's range
  ret = ret && connect(levelHandle, SIGNAL(xshLevelChanged()), this,
                       SLOT(updateFrameRange()));

  // onFrameTypeChanged(): reset the marker positions in the flip console
  ret = ret && connect(frameHandle, SIGNAL(frameTypeChanged()), this,
                       SLOT(onFrameTypeChanged()));

  // onXshLevelSwitched(TXshLevel*)�F changeWindowTitle() + updateFrameRange()
  ret = ret && connect(levelHandle, SIGNAL(xshLevelSwitched(TXshLevel *)), this,
                       SLOT(onXshLevelSwitched(TXshLevel *)));

  // onFrameSwitched(): update the flipconsole according to the current frame
  ret = ret && connect(frameHandle, SIGNAL(frameSwitched()), this,
                       SLOT(onFrameSwitched()));

  ret = ret && connect(app->getCurrentTool(), SIGNAL(toolSwitched()),
                       m_sceneViewer, SLOT(onToolSwitched()));
  ret =
      ret && connect(sceneHandle, SIGNAL(preferenceChanged(const QString &)),
                     m_flipConsole, SLOT(onPreferenceChanged(const QString &)));

  assert(ret);

  m_sceneViewer->onToolSwitched();

  m_flipConsole->setActive(true);
  m_flipConsole->onPreferenceChanged("");

  // refresh
  onSceneChanged();
  changeWindowTitle();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::hideEvent(QHideEvent *event) {
  TApp *app = TApp::instance();
  disconnect(app->getCurrentFrame(), nullptr, this, nullptr);
  disconnect(app->getCurrentScene(), nullptr, this, nullptr);
  disconnect(app->getCurrentLevel(), nullptr, this, nullptr);
  disconnect(app->getCurrentXsheet(), nullptr, this, nullptr);

  m_flipConsole->setActive(false);
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::initializeTitleBar(TPanelTitleBar *titleBar) {
  m_titleBar = titleBar;

  bool ret = true;

  TPanelTitleBarButtonSet *viewModeButtonSet;
  m_referenceModeBs = viewModeButtonSet = new TPanelTitleBarButtonSet();
  TPanelTitleBarButton *button;

  //------
  // buttons for show / hide toggle for the field guide and the safe area
  TPanelTitleBarButtonForSafeArea *safeAreaButton =
      new TPanelTitleBarButtonForSafeArea(titleBar, getIconPath("pane_safe"));
  safeAreaButton->setToolTip(tr("Safe Area (Right Click to Select)"));
  ret = ret && connect(safeAreaButton, SIGNAL(toggled(bool)),
                       CommandManager::instance()->getAction(MI_SafeArea),
                       SLOT(trigger()));
  ret = ret && connect(CommandManager::instance()->getAction(MI_SafeArea),
                       SIGNAL(triggered(bool)), safeAreaButton,
                       SLOT(setPressed(bool)));
  // initialize state
  safeAreaButton->setPressed(
      CommandManager::instance()->getAction(MI_SafeArea)->isChecked());
  m_titleBarButtons.insert(VTBButtons_SafeArea, safeAreaButton);

  button = new TPanelTitleBarButton(titleBar, getIconPath("pane_grid"));

  button->setToolTip(tr("Field Guide"));
  ret = ret && connect(button, SIGNAL(toggled(bool)),
                       CommandManager::instance()->getAction(MI_FieldGuide),
                       SLOT(trigger()));
  ret = ret && connect(CommandManager::instance()->getAction(MI_FieldGuide),
                       SIGNAL(triggered(bool)), button, SLOT(setPressed(bool)));
  // initialize state
  button->setPressed(
      CommandManager::instance()->getAction(MI_FieldGuide)->isChecked());
  m_titleBarButtons.insert(VTBButtons_FieldGuide, button);

  //------
  // view mode toggles
  button = new TPanelTitleBarButton(titleBar, getIconPath("pane_table"));
  button->setToolTip(tr("Camera Stand View"));
  button->setButtonSet(viewModeButtonSet, SceneViewer::NORMAL_REFERENCE);
  button->setPressed(true);
  m_titleBarButtons.insert(VTBButtons_ViewModeCamStand, button);

  button = new TPanelTitleBarButton(titleBar, getIconPath("pane_3d"));
  button->setToolTip(tr("3D View"));
  button->setButtonSet(viewModeButtonSet, SceneViewer::CAMERA3D_REFERENCE);
  m_titleBarButtons.insert(VTBButtons_ViewMode3D, button);

  button = new TPanelTitleBarButton(titleBar, getIconPath("pane_cam"));
  button->setToolTip(tr("Camera View"));
  button->setButtonSet(viewModeButtonSet, SceneViewer::CAMERA_REFERENCE);
  ret = ret && connect(viewModeButtonSet, SIGNAL(selected(int)), m_sceneViewer,
                       SLOT(setReferenceMode(int)));
  m_titleBarButtons.insert(VTBButtons_ViewModeCamera, button);

  //------
  // freeze button
  button = new TPanelTitleBarButton(titleBar, getIconPath("pane_freeze"));
  button->setToolTip(tr("Freeze"));
  ret = ret && connect(button, SIGNAL(toggled(bool)), m_sceneViewer,
                       SLOT(freeze(bool)));
  m_titleBarButtons.insert(VTBButtons_Freeze, button);

  //------
  // preview toggles
  m_previewButton = new TPanelTitleBarButtonForPreview(titleBar, getIconPath("pane_preview"));
  m_previewButton->setToolTip(tr("Preview"));
  m_titleBarButtons.insert(VTBButtons_Preview, m_previewButton);

  //------
  m_subcameraPreviewButton = new TPanelTitleBarButtonForPreview(titleBar, getIconPath("pane_subpreview"));
  m_subcameraPreviewButton->setToolTip(tr("Sub-camera Preview"));
  m_titleBarButtons.insert(VTBButtons_SubPreview, m_subcameraPreviewButton);

  //------
  QToolButton *hamburger = new QToolButton(titleBar);
  hamburger->setIcon(createQIcon("menu"));
  hamburger->setObjectName("ViewerTitleBarCustomize");
  hamburger->setIconSize(QSize(16, 16));
  hamburger->setFixedSize(20, 20);
  hamburger->setPopupMode(QToolButton::MenuButtonPopup);
  QMenu *menu = new QMenu();
  hamburger->setMenu(menu);
  addTitleBarMenuItem(VTBButtons_SafeArea, tr("Safe Area"), menu);
  addTitleBarMenuItem(VTBButtons_FieldGuide, tr("Field Guide"), menu);
  menu->addSeparator();
  addTitleBarMenuItem(VTBButtons_ViewModeCamStand, tr("Camera Stand View"),
                      menu);
  addTitleBarMenuItem(VTBButtons_ViewMode3D, tr("3D View"), menu);
  addTitleBarMenuItem(VTBButtons_ViewModeCamera, tr("Camera View"), menu);
  menu->addSeparator();
  addTitleBarMenuItem(VTBButtons_Freeze, tr("Freeze"), menu);
  menu->addSeparator();
  addTitleBarMenuItem(VTBButtons_Preview, tr("Preview"), menu);
  addTitleBarMenuItem(VTBButtons_SubPreview, tr("Sub-camera Preview"), menu);

  ret = connect(menu, SIGNAL(triggered(QAction *)), this,
                SLOT(onCustomizeTitleBarPressed(QAction *)));
  m_titleBarButtons.insert(VTBButtons_Hamburger, hamburger);

  placeTitleBarButtons();
  assert(ret);
}

//----------------------------------------------------------------------------------------------

void BaseViewerPanel::addTitleBarMenuItem(UINT id, const QString &text,
                                          QMenu *menu) {
  QAction *a = new QAction(text, menu);
  a->setCheckable(true);
  a->setData(QVariant(id));
  menu->addAction(a);
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::placeTitleBarButtons() {
  const QMap<VTitleBar_Buttons, int> widthMap = {
      {VTBButtons_SafeArea, 20},         {VTBButtons_FieldGuide, 20},
      {VTBButtons_ViewModeCamStand, 20}, {VTBButtons_ViewMode3D, 20},
      {VTBButtons_ViewModeCamera, 20},   {VTBButtons_Freeze, 20},
      {VTBButtons_Preview, 24},          {VTBButtons_SubPreview, 24},
      {VTBButtons_Hamburger, 20}};
  // buttons which need space on the left
  const QList<VTitleBar_Buttons> sepButtons = {
      VTBButtons_ViewModeCamStand, VTBButtons_Freeze, VTBButtons_Preview,
      VTBButtons_Hamburger};

  m_titleBar->clearButtons();

  // place the buttons from the right
  int pos           = 21;  // keep space for the close button
  UINT buttonId     = VTBButtons_Hamburger;
  bool separateFlag = false;
  while (buttonId != VTBButtons_None) {
    QWidget *widget = m_titleBarButtons.value((VTitleBar_Buttons)buttonId);
    if (m_titleBarMask & buttonId) {
      if (separateFlag) {
        pos += 10;
        separateFlag = false;
      } else
        pos += 1;

      pos += widthMap.value((VTitleBar_Buttons)buttonId);

      // register to the title bar
      m_titleBar->add(QPoint(-pos, 0), widget);
      widget->setVisible(true);
    } else
      widget->setVisible(false);

    if (sepButtons.contains((VTitleBar_Buttons)buttonId)) separateFlag = true;

    buttonId = buttonId >> 1;  // next id
  }

  m_titleBar->placeButtons();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::getPreviewButtonStates(bool &prev, bool &subCamPrev) {
  prev       = m_previewButton->isChecked();
  subCamPrev = m_subcameraPreviewButton->isChecked();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::enableFullPreview(bool enabled) {
  m_subcameraPreviewButton->setPressed(false);
  if (CommandManager::instance()
          ->getAction(MI_ToggleViewerSubCameraPreview)
          ->isChecked())
    CommandManager::instance()
        ->getAction(MI_ToggleViewerSubCameraPreview)
        ->setChecked(false);

  if (!enabled && EnvViewerPreviewBehavior == 2 &&
      FlipConsole::getCurrent() == m_flipConsole &&
      TApp::instance()->getCurrentFrame()->isPlaying())
    CommandManager::instance()->execute(MI_Pause);

  m_sceneViewer->enablePreview(enabled ? SceneViewer::FULL_PREVIEW
                                       : SceneViewer::NO_PREVIEW);
  m_flipConsole->setProgressBarStatus(
      &Previewer::instance(false)->getProgressBarStatus());
  enableFlipConsoleForCamerastand(enabled);
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::enableSubCameraPreview(bool enabled) {
  m_previewButton->setPressed(false);
  if (CommandManager::instance()
          ->getAction(MI_ToggleViewerPreview)
          ->isChecked())
    CommandManager::instance()
        ->getAction(MI_ToggleViewerPreview)
        ->setChecked(false);

  if (!enabled && EnvViewerPreviewBehavior == 2 &&
      FlipConsole::getCurrent() == m_flipConsole &&
      TApp::instance()->getCurrentFrame()->isPlaying())
    CommandManager::instance()->execute(MI_Pause);

  m_sceneViewer->enablePreview(enabled ? SceneViewer::SUBCAMERA_PREVIEW
                                       : SceneViewer::NO_PREVIEW);
  m_flipConsole->setProgressBarStatus(
      &Previewer::instance(true)->getProgressBarStatus());
  enableFlipConsoleForCamerastand(enabled);
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::enableFlipConsoleForCamerastand(bool on) {
  m_flipConsole->enableButton(FlipConsole::eMatte, on, false);
  m_flipConsole->enableButton(FlipConsole::eSave, on, false);
  m_flipConsole->enableButton(FlipConsole::eCompare, on, false);
  m_flipConsole->enableButton(FlipConsole::eSaveImg, on, false);
  m_flipConsole->enableButton(FlipConsole::eGRed, on, false);
  m_flipConsole->enableButton(FlipConsole::eGGreen, on, false);
  m_flipConsole->enableButton(FlipConsole::eGBlue, on, false);
  m_flipConsole->enableButton(FlipConsole::eBlackBg, on, false);
  m_flipConsole->enableButton(FlipConsole::eWhiteBg, on, false);
  m_flipConsole->enableButton(FlipConsole::eCheckBg, on, false);

  m_flipConsole->enableProgressBar(on);
  update();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::onXshLevelSwitched(TXshLevel *) {
  changeWindowTitle();
  m_sceneViewer->update();
  // If the level is switched by using the combobox in the film strip, the
  // current level switches without change in the frame type (level or scene).
  // For such case, update the frame range of the console here.
  if (TApp::instance()->getCurrentFrame()->isEditingLevel()) updateFrameRange();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::onPlayingStatusChanged(bool playing) {
  if (playing) {
    m_playing = true;
  } else {
    m_playing = false;
    m_first   = true;
  }

  // if preview behavior mode is "selected cells", release preview mode when
  // stopped
  if (!playing && FlipConsole::getCurrent() == m_flipConsole) {
    if (Preferences::instance()->previewWhenPlayingOnViewerEnabled() ||
        (EnvViewerPreviewBehavior == 2 &&
         !Previewer::instance(m_sceneViewer->getPreviewMode() ==
                              SceneViewer::SUBCAMERA_PREVIEW)
              ->isBusy())) {
      if (CommandManager::instance()
              ->getAction(MI_ToggleViewerPreview)
              ->isChecked())
        CommandManager::instance()
            ->getAction(MI_ToggleViewerPreview)
            ->trigger();
      else if (CommandManager::instance()
                   ->getAction(MI_ToggleViewerSubCameraPreview)
                   ->isChecked())
        CommandManager::instance()
            ->getAction(MI_ToggleViewerSubCameraPreview)
            ->trigger();
    }
  }

  if (Preferences::instance()->getOnionSkinDuringPlayback()) return;
  OnionSkinMask osm =
      TApp::instance()->getCurrentOnionSkin()->getOnionSkinMask();
  if (playing) {
    m_onionSkinActive = osm.isEnabled();
    if (m_onionSkinActive) {
      osm.enable(false);
      TApp::instance()->getCurrentOnionSkin()->setOnionSkinMask(osm);
      TApp::instance()->getCurrentOnionSkin()->notifyOnionSkinMaskChanged();
    }
  } else {
    if (m_onionSkinActive) {
      osm.enable(true);
      TApp::instance()->getCurrentOnionSkin()->setOnionSkinMask(osm);
      TApp::instance()->getCurrentOnionSkin()->notifyOnionSkinMaskChanged();
    }
  }
  m_sceneViewer->invalidateToolStatus();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::changeWindowTitle() {  // �v�m�F
  TApp *app         = TApp::instance();
  ToonzScene *scene = app->getCurrentScene()->getScene();
  if (!scene) return;
  if (!parentWidget()) return;
  int frame = app->getCurrentFrame()->getFrame();

  // put the titlebar texts in this string
  QString name;

  // if the frame type is "scene editing"
  if (app->getCurrentFrame()->isEditingScene()) {
    TProject *project = scene->getProject();
    QString sceneName = QString::fromStdWString(scene->getSceneName());
    if (sceneName.isEmpty()) sceneName = tr("Untitled");
    if (app->getCurrentScene()->getDirtyFlag()) sceneName += QString("*");
    name = tr("Scene: ") + sceneName;
    if (frame >= 0)
      name =
          name + tr("   ::   Frame: ") + tr(std::to_string(frame + 1).c_str());
    int col = app->getCurrentColumn()->getColumnIndex();
    if (col < 0) {
      parentWidget()->setWindowTitle(name);
      return;
    }
    TXsheet *xsh = app->getCurrentXsheet()->getXsheet();
    TXshCell cell;
    if (app->getCurrentColumn()->getColumn() &&
        !app->getCurrentColumn()->getColumn()->getSoundColumn())
      cell = xsh->getCell(frame, col);
    if (cell.isEmpty()) {
      if (!m_sceneViewer->is3DView()) {
        TAffine aff = m_sceneViewer->getViewMatrix() *
                      m_sceneViewer->getNormalZoomScale().inv();
        if (m_sceneViewer->getIsFlippedX()) aff = aff * TScale(-1, 1);
        if (m_sceneViewer->getIsFlippedY()) aff = aff * TScale(1, -1);
        name = name + tr("  ::  Zoom : ") +
               QString::number(tround(100.0 * sqrt(aff.det()))) + "%";
        if (m_sceneViewer->getIsFlippedX() || m_sceneViewer->getIsFlippedY()) {
          name = name + tr(" (Flipped)");
        }
      }
      parentWidget()->setWindowTitle(name);
      return;
    }
    assert(cell.m_level.getPointer());
    TFilePath fp(cell.m_level->getName());
    QString imageName =
        QString::fromStdWString(fp.withFrame(cell.m_frameId).getWideString());
    name = name + tr("   ::   Level: ") + imageName;
  }
  // if the frame type is "level editing"
  else {
    TXshLevel *level = app->getCurrentLevel()->getLevel();
    if (level) {
      TFilePath fp(level->getName());
      QString imageName = QString::fromStdWString(
          fp.withFrame(app->getCurrentFrame()->getFid()).getWideString());
      name = name + tr("Level: ") + imageName;
    }
  }
  if (!m_sceneViewer->is3DView()) {
    TAffine aff = m_sceneViewer->getSceneMatrix() *
                  m_sceneViewer->getNormalZoomScale().inv();
    if (m_sceneViewer->getIsFlippedX()) aff = aff * TScale(-1, 1);
    if (m_sceneViewer->getIsFlippedY()) aff = aff * TScale(1, -1);
    name = name + tr("  ::  Zoom : ") +
           QString::number(tround(100.0 * sqrt(aff.det()))) + "%";
    if (m_sceneViewer->getIsFlippedX() || m_sceneViewer->getIsFlippedY()) {
      name = name + tr(" (Flipped)");
    }
  }

  parentWidget()->setWindowTitle(name);
}

//-----------------------------------------------------------------------------
/*! update the frame range according to the current frame type
 */
void BaseViewerPanel::updateFrameRange() {
  TFrameHandle *fh  = TApp::instance()->getCurrentFrame();
  int frameIndex    = fh->getFrameIndex();
  int maxFrameIndex = fh->getMaxFrameIndex();
  if (frameIndex > maxFrameIndex) maxFrameIndex = frameIndex;
  m_flipConsole->setFrameRange(1, maxFrameIndex + 1, 1, frameIndex + 1);
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::updateFrameMarkers() {
  int fromIndex, toIndex, dummy;
  XsheetGUI::getPlayRange(fromIndex, toIndex, dummy);
  TFrameHandle *fh = TApp::instance()->getCurrentFrame();
  if (fh->isEditingLevel()) {
    fromIndex = 0;
    toIndex   = -1;
  }
  m_flipConsole->setMarkers(fromIndex, toIndex);
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::onSceneChanged() {
  updateFrameRange();
  updateFrameMarkers();
  changeWindowTitle();
  TApp *app         = TApp::instance();
  ToonzScene *scene = app->getCurrentScene()->getScene();
  assert(scene);
  // update fps only when the scene settings is changed
  m_flipConsole->setFrameRate(TApp::instance()
                                  ->getCurrentScene()
                                  ->getScene()
                                  ->getProperties()
                                  ->getOutputProperties()
                                  ->getFrameRate(),
                              false);

  int frameIndex = TApp::instance()->getCurrentFrame()->getFrameIndex();
  if (m_keyFrameButton->getCurrentFrame() != frameIndex)
    m_keyFrameButton->setCurrentFrame(frameIndex);
  hasSoundtrack();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::onSceneSwitched() {
  m_previewButton->setPressed(false);
  m_subcameraPreviewButton->setPressed(false);
  enableFlipConsoleForCamerastand(false);
  m_sceneViewer->enablePreview(SceneViewer::NO_PREVIEW);
  m_flipConsole->setChecked(FlipConsole::eDefineSubCamera, false);
  m_flipConsole->setFrameRate(TApp::instance()
                                  ->getCurrentScene()
                                  ->getScene()
                                  ->getProperties()
                                  ->getOutputProperties()
                                  ->getFrameRate());
  m_sceneViewer->setEditPreviewSubcamera(false);
  onSceneChanged();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::onFrameSwitched() {
  int frameIndex = TApp::instance()->getCurrentFrame()->getFrameIndex();
  m_flipConsole->setCurrentFrame(frameIndex + 1);
  if (m_keyFrameButton->getCurrentFrame() != frameIndex)
    m_keyFrameButton->setCurrentFrame(frameIndex);

  if (m_playing && m_playSound) {
    if (m_first == true && hasSoundtrack()) {
      playAudioFrame(frameIndex);
    } else if (m_hasSoundtrack) {
      playAudioFrame(frameIndex);
    }
  }
}

//-----------------------------------------------------------------------------
/*! reset the marker positions in the flip console
 */
void BaseViewerPanel::onFrameTypeChanged() {
  if (TApp::instance()->getCurrentFrame()->getFrameType() ==
      TFrameHandle::LevelFrame) {
    if (m_sceneViewer->isPreviewEnabled()) {
      m_previewButton->setPressed(false);
      m_subcameraPreviewButton->setPressed(false);
      enableFlipConsoleForCamerastand(false);
      m_sceneViewer->enablePreview(SceneViewer::NO_PREVIEW);
    }
    CameraTestCheck::instance()->setIsEnabled(false);
    CleanupPreviewCheck::instance()->setIsEnabled(false);
  }

  m_flipConsole->setChecked(FlipConsole::eDefineSubCamera, false);
  m_sceneViewer->setEditPreviewSubcamera(false);

  updateFrameRange();
  updateFrameMarkers();
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::playAudioFrame(int frame) {
  if (m_first) {
    m_first = false;
    m_fps   = TApp::instance()
                ->getCurrentScene()
                ->getScene()
                ->getProperties()
                ->getOutputProperties()
                ->getFrameRate();
    m_samplesPerFrame = m_sound->getSampleRate() / std::abs(m_fps);
  }
  if (!m_sound) return;
  m_viewerFps = m_flipConsole->getCurrentFps();
  double s0 = frame * m_samplesPerFrame, s1 = s0 + m_samplesPerFrame;

  // make the sound stop if the viewerfps is higher so the next sound can play
  // on time.
  if (m_fps < m_viewerFps)
    TApp::instance()->getCurrentXsheet()->getXsheet()->stopScrub();
  TApp::instance()->getCurrentXsheet()->getXsheet()->play(m_sound, s0, s1,
                                                          false);
}

//-----------------------------------------------------------------------------

bool BaseViewerPanel::hasSoundtrack() {
  if (m_sound != NULL) {
    m_sound         = NULL;
    m_hasSoundtrack = false;
    m_first         = true;
  }
  TXsheetHandle *xsheetHandle    = TApp::instance()->getCurrentXsheet();
  TXsheet::SoundProperties *prop = new TXsheet::SoundProperties();
  if (!m_sceneViewer->isPreviewEnabled()) prop->m_isPreview = true;
  try {
    m_sound = xsheetHandle->getXsheet()->makeSound(prop);
  } catch (TSoundDeviceException &e) {
    if (e.getType() == TSoundDeviceException::NoDevice) {
      std::cout << ::to_string(e.getMessage()) << std::endl;
    } else {
      throw TSoundDeviceException(e.getType(), e.getMessage());
    }
  }
  if (m_sound == NULL) {
    m_hasSoundtrack = false;
    return false;
  } else {
    m_hasSoundtrack = true;
    return true;
  }
}

void BaseViewerPanel::onButtonPressed(FlipConsole::EGadget button) {
  if (button == FlipConsole::eSound) {
    m_playSound = !m_playSound;
  }
}

void BaseViewerPanel::setFlipHButtonChecked(bool checked) {
  m_flipConsole->setChecked(FlipConsole::eFlipHorizontal, checked);
}

void BaseViewerPanel::setFlipVButtonChecked(bool checked) {
  m_flipConsole->setChecked(FlipConsole::eFlipVertical, checked);
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::setVisiblePartsFlag(UINT flag) {
  m_visiblePartsFlag = flag;
  updateShowHide();
}

// SaveLoadQSettings
void BaseViewerPanel::save(QSettings &settings) const {
  settings.setValue("viewerVisibleParts", m_visiblePartsFlag);
  settings.setValue("titleBarVisibleButtons", m_titleBarMask);
}

void BaseViewerPanel::load(QSettings &settings) {
  checkOldVersionVisblePartsFlags(settings);
  m_visiblePartsFlag =
      settings.value("viewerVisibleParts", m_visiblePartsFlag).toUInt();
  m_titleBarMask =
      settings.value("titleBarVisibleButtons", m_titleBarMask).toUInt();
  updateShowHide();
  placeTitleBarButtons();

  // update visibility menu
  QMenu *menu =
      qobject_cast<QToolButton *>(m_titleBarButtons[VTBButtons_Hamburger])
          ->menu();
  for (auto a : menu->actions()) {
    UINT id = a->data().toUInt();
    a->setChecked(id & m_titleBarMask);
  }
}

//-----------------------------------------------------------------------------

void BaseViewerPanel::onPreviewStatusChanged() {
  if (FlipConsole::getCurrent() == m_flipConsole &&
      !TApp::instance()->getCurrentFrame()->isPlaying() &&
      m_sceneViewer->isPreviewEnabled() &&
      !Previewer::instance(m_sceneViewer->getPreviewMode() ==
                           SceneViewer::SUBCAMERA_PREVIEW)
           ->isBusy()) {
    int buttonId           = 0;
    CommandId playCmdId    = MI_Loop;
    CommandId previewCmdId = MI_ToggleViewerPreview;
    if (Preferences::instance()->previewWhenPlayingOnViewerEnabled() &&
        m_sceneViewer->isPreviewEnabled()) {
      previewCmdId =
          (m_sceneViewer->getPreviewMode() == SceneViewer::FULL_PREVIEW)
              ? MI_ToggleViewerPreview
              : MI_ToggleViewerSubCameraPreview;
      if (CommandManager::instance()->getAction(previewCmdId)->data().toInt() !=
          0) {
        buttonId =
            CommandManager::instance()->getAction(previewCmdId)->data().toInt();
        playCmdId = (buttonId == FlipConsole::ePlay) ? MI_Play : MI_Loop;
      }
    }

    // current frame
    if (buttonId && EnvViewerPreviewBehavior == 0) {
      CommandManager::instance()->execute(playCmdId);
      CommandManager::instance()->getAction(previewCmdId)->setData(QVariant());
    }
    // all frames
    else if (buttonId && EnvViewerPreviewBehavior == 1) {
      int r0, r1, step;
      ToonzScene *scene = TApp::instance()->getCurrentScene()->getScene();
      scene->getProperties()->getPreviewProperties()->getRange(r0, r1, step);
      if (r0 > r1) {
        r0 = 0;
        r1 = scene->getFrameCount() - 1;
      }
      for (int f = r0; f <= r1; f += step) {
        if (!Previewer::instance(m_sceneViewer->getPreviewMode() ==
                                 SceneViewer::SUBCAMERA_PREVIEW)
                 ->isFrameReady(f)) {
          update();
          return;
        }
      }
      CommandManager::instance()->execute(playCmdId);
      CommandManager::instance()->getAction(previewCmdId)->setData(QVariant());
    }
    // if preview behavior mode is "selected cells", play once the all frames
    // are completed
    else if (EnvViewerPreviewBehavior == 2) {
      TCellSelection *cellSel =
          dynamic_cast<TCellSelection *>(TSelection::getCurrent());
      if (cellSel && !cellSel->isEmpty()) {
        int r0, c0, r1, c1;
        cellSel->getSelectedCells(r0, c0, r1, c1);
        if (r0 < r1) {
          // check if all frame range is rendered. this check is needed since
          // isBusy() will not be true just after the preview is triggered
          for (int r = r0; r <= r1; r++) {
            if (!Previewer::instance(m_sceneViewer->getPreviewMode() ==
                                     SceneViewer::SUBCAMERA_PREVIEW)
                     ->isFrameReady(r)) {
              update();
              return;
            }
          }
          m_flipConsole->setStopAt(r1 + 1);
          m_flipConsole->setStartAt(r0 + 1);
          TApp::instance()->getCurrentFrame()->setFrame(r0);
          CommandManager::instance()->execute(playCmdId);
          CommandManager::instance()
              ->getAction(previewCmdId)
              ->setData(QVariant());
        }
      }
    }
  }

  update();
}

//-----------------------------------------------------------------------------
// sync preview commands and buttons states when the viewer becomes active

void BaseViewerPanel::onActiveViewerChanged() {
  bool ret = true;
  if (TApp::instance()->getActiveViewer() == m_sceneViewer) {
    ret = ret &&
          connect(m_previewButton, SIGNAL(toggled(bool)),
                  CommandManager::instance()->getAction(MI_ToggleViewerPreview),
                  SLOT(trigger()));
    ret = ret &&
          connect(CommandManager::instance()->getAction(MI_ToggleViewerPreview),
                  SIGNAL(triggered(bool)), m_previewButton,
                  SLOT(setPressed(bool)));
    ret        = ret && connect(m_subcameraPreviewButton, SIGNAL(toggled(bool)),
                                CommandManager::instance()->getAction(
                             MI_ToggleViewerSubCameraPreview),
                                SLOT(trigger()));
    ret        = ret && connect(CommandManager::instance()->getAction(
                             MI_ToggleViewerSubCameraPreview),
                                SIGNAL(triggered(bool)), m_subcameraPreviewButton,
                                SLOT(setPressed(bool)));
    m_isActive = true;
  } else if (m_isActive) {
    ret = ret && disconnect(m_previewButton, SIGNAL(toggled(bool)),
                            CommandManager::instance()->getAction(
                                MI_ToggleViewerPreview),
                            SLOT(trigger()));
    ret = ret &&
          disconnect(
              CommandManager::instance()->getAction(MI_ToggleViewerPreview),
              SIGNAL(triggered(bool)), m_previewButton, SLOT(setPressed(bool)));
    ret = ret && disconnect(m_subcameraPreviewButton, SIGNAL(toggled(bool)),
                            CommandManager::instance()->getAction(
                                MI_ToggleViewerSubCameraPreview),
                            SLOT(trigger()));
    ret = ret && disconnect(CommandManager::instance()->getAction(
                                MI_ToggleViewerSubCameraPreview),
                            SIGNAL(triggered(bool)), m_subcameraPreviewButton,
                            SLOT(setPressed(bool)));
    m_isActive = false;
  }
  assert(ret);
}

//=============================================================================
//
// SceneViewerPanel
//
//-----------------------------------------------------------------------------

SceneViewerPanel::SceneViewerPanel(QWidget *parent, Qt::WindowFlags flags)
    : BaseViewerPanel(parent, flags) {
  setObjectName("ViewerPanel");
  setMinimumHeight(200);

  Ruler *vRuler = new Ruler(this, m_sceneViewer, true);
  Ruler *hRuler = new Ruler(this, m_sceneViewer, false);
  m_sceneViewer->setRulers(vRuler, hRuler);

  {
    QGridLayout *viewerL = new QGridLayout();
    viewerL->setMargin(0);
    viewerL->setSpacing(0);
    {
      viewerL->addWidget(vRuler, 1, 0);
      viewerL->addWidget(hRuler, 0, 1);
      viewerL->addWidget(m_fsWidget, 1, 1);
    }
    viewerL->setRowStretch(1, 1);
    viewerL->setColumnStretch(1, 1);
    m_mainLayout->insertLayout(0, viewerL, 1);
  }
  setLayout(m_mainLayout);
  // initial state of the parts
  m_visiblePartsFlag = VPPARTS_ALL;
  updateShowHide();
}

//-----------------------------------------------------------------------------

void SceneViewerPanel::checkOldVersionVisblePartsFlags(QSettings &settings) {
  if (settings.contains("viewerVisibleParts") ||
      !settings.contains("visibleParts"))
    return;
  UINT oldVisiblePartsFlag =
      settings.value("visibleParts", OldVPARTS_ALL).toUInt();
  m_visiblePartsFlag = VPPARTS_None;
  if (oldVisiblePartsFlag & OldVPARTS_PLAYBAR)
    m_visiblePartsFlag |= VPPARTS_PLAYBAR;
  if (oldVisiblePartsFlag & OldVPARTS_FRAMESLIDER)
    m_visiblePartsFlag |= VPPARTS_FRAMESLIDER;
}

//=========================================================

class ViewerPreviewCommands : public QObject {
public:
  ViewerPreviewCommands() {
    setCommandHandler("MI_ToggleViewerPreview", this,
                      &ViewerPreviewCommands::onPreview);
    setCommandHandler("MI_ToggleViewerSubCameraPreview", this,
                      &ViewerPreviewCommands::onSubCameraPreview);
  }

  void onPreview();
  void onSubCameraPreview();
};

void ViewerPreviewCommands::onPreview() {
  SceneViewer *activeViewer = TApp::instance()->getActiveViewer();
  if (!activeViewer) return;
  BaseViewerPanel *bvp = qobject_cast<BaseViewerPanel *>(
      activeViewer->parentWidget()->parentWidget());
  if (!bvp) return;
  bool on = CommandManager::instance()
                ->getAction(MI_ToggleViewerPreview)
                ->isChecked();
  bvp->enableFullPreview(on);
}

void ViewerPreviewCommands::onSubCameraPreview() {
  SceneViewer *activeViewer = TApp::instance()->getActiveViewer();
  if (!activeViewer) return;
  BaseViewerPanel *bvp = qobject_cast<BaseViewerPanel *>(
      activeViewer->parentWidget()->parentWidget());
  if (!bvp) return;
  bool on = CommandManager::instance()
                ->getAction(MI_ToggleViewerSubCameraPreview)
                ->isChecked();
  bvp->enableSubCameraPreview(on);
}

ViewerPreviewCommands viewerPreviewCommands;
