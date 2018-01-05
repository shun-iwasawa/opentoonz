
// Toonz includes
#include "tapp.h"

// TnzTools includes
#include "tools/toolhandle.h"
#include "tools/cursormanager.h"

// TnzLib includes
#include "toonz/tscenehandle.h"
#include "toonz/tframehandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/txshlevelhandle.h"
#include "toonz/tonionskinmaskhandle.h"
#include "toonz/tcolumnhandle.h"
#include "toonz/preferences.h"
#include "toonz/txsheet.h"
#include "toonz/tstageobjecttree.h"
#include "toonz/stage2.h"
#include "toonz/stagevisitor.h"
#include "toonz/openglviewerdraw.h"
#include "toonz/toonzscene.h"
#include "toonz/tcamera.h"
#include "toonz/txshsimplelevel.h"


#include "openglsceneviewer.h"

// for debug purpose
namespace {
  void printMatrix(const QMatrix4x4& m) {
    for (int j = 0; j < 4; j++) {
      for (int i = 0; i < 4; i++) {
        std::cout << m.row(j)[i] << ", ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }


};

//-----------------------------------------------------------------------------

OpenGLSceneViewer::OpenGLSceneViewer(QWidget *parent)
  : OpenGLWidgetForHighDpi(parent) {

  for (int i = 0; i < 2; i++)
    m_viewMatrix[i]= OpenGLViewerDraw::toQMatrix(getNormalZoomScale());
  // set the root model matrix
  m_modelMatrix.push(QMatrix4x4());
}

//-----------------------------------------------------------------------------

OpenGLSceneViewer::~OpenGLSceneViewer() {

}

//-----------------------------------------------------------------------------

void OpenGLSceneViewer::invalidateToolStatus() {
  TTool *tool = TApp::instance()->getCurrentTool()->getTool();
  if (tool) {
    m_toolDisableReason = tool->updateEnabled();
    if (tool->isEnabled()) {
      setToolCursor(this, tool->getCursorId());
      tool->setViewer(this);
      tool->updateMatrix();
    }
    else
      setCursor(Qt::ForbiddenCursor);
  }
  else
    setCursor(Qt::ForbiddenCursor);
}

//-----------------------------------------------------------------------------

TRect OpenGLSceneViewer::getActualClipRect(const TAffine &aff) {
  TDimension viewerSize(width(), height());
  TRect clipRect(viewerSize);

  /*if (is3DView()) { //TODO
    TPointD p00 = winToWorld(clipRect.getP00());
    TPointD p01 = winToWorld(clipRect.getP01());
    TPointD p10 = winToWorld(clipRect.getP10());
    TPointD p11 = winToWorld(clipRect.getP11());
    clipRect = TRect(TPoint(std::min(p00.x, p01.x), std::min(p00.y, p10.y)),
      TPoint(std::max(p11.x, p10.x), std::max(p11.y, p01.y)));
  }
  else*/
  if (m_clipRect.isEmpty())
    clipRect -= TPoint(viewerSize.lx / 2, viewerSize.ly / 2);
  else {
    TRectD app = aff * (m_clipRect.enlarge(3));
    clipRect =
      TRect(tceil(app.x0), tceil(app.y0), tfloor(app.x1), tfloor(app.y1));
  }

  return clipRect;
}

//-----------------------------------------------------------------------------

TAffine OpenGLSceneViewer::getViewMatrix() const {
  return OpenGLViewerDraw::toTAffine(getViewQMatrix());
}

//-----------------------------------------------------------------------------

QMatrix4x4 OpenGLSceneViewer::getViewQMatrix() const {
  int viewMode = (int)m_viewMode;
  if (is3DView()) return QMatrix4x4();
  if (m_referenceMode == CAMERA_REFERENCE) {
    int frame = TApp::instance()->getCurrentFrame()->getFrame();
    TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
    TAffine aff = xsh->getCameraAff(frame);
    return m_viewMatrix[viewMode] * OpenGLViewerDraw::toQMatrix(aff.inv());
  }
  else
    return m_viewMatrix[viewMode];
}

//-----------------------------------------------------------------------------

TPoint OpenGLSceneViewer::worldToPos(const TPointD &worldPos) const {
  TPointD p = getViewMatrix() * worldPos;
  return TPoint(width() / 2 + p.x, height() / 2 + p.y);
}

//-----------------------------------------------------------------------------

bool OpenGLSceneViewer::is3DView() const {
  bool isCameraTest = CameraTestCheck::instance()->isEnabled();
  return (m_referenceMode == CAMERA3D_REFERENCE && !isCameraTest);
}

//-----------------------------------------------------------------------------

void OpenGLSceneViewer::initializeGL() {
  initializeOpenGLFunctions();
  OpenGLViewerDraw::instance()->initialize();
}

//-----------------------------------------------------------------------------

void OpenGLSceneViewer::resizeGL(int w, int h) {
  w *= getDevPixRatio();
  h *= getDevPixRatio();
  glViewport(0, 0, w, h);
  // set the projection matrix
  m_projectionMatrix.setToIdentity();
  m_projectionMatrix.ortho(0, w, 0, h, -4000, 4000);

  m_modelMatrix.top() = QMatrix4x4();
  m_modelMatrix.top().translate(0.375f, 0.375f, 0.0f);
  m_modelMatrix.top().translate((float)w * 0.5f, (float)h * 0.5f, 0.0f);
}

//-----------------------------------------------------------------------------

void OpenGLSceneViewer::paintGL() {
  // Clear the screen
  // drawBackground() is replaced by updateBackgroundColor()
  glClearColor(m_bgColor[0], m_bgColor[1], m_bgColor[2], 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);


  //just like drawBuildVars()
  setupPlacements();

  //copyFrontBufferToBackBuffer();
  
  //drawEnableScissor();
  

  if (m_previewMode != FULL_PREVIEW) {
    drawCameraStand();
  }

  //if (isPreviewEnabled()) drawPreview();

  //drawOverlay();

  //drawDisableScissor();
}

//-----------------------------------------------------------------------------
// update clear color according to the viewer state and preferences
//-----------------------------------------------------------------------------
//TODO : これをm_previewModeが変更された直後に必ず呼ぶ
void OpenGLSceneViewer::updateBackgroundColor() {
  if (1) {
  //if (m_visualSettings.m_colorMask == 0) {
    TPixel32 bgColor;
    if (isPreviewEnabled())
      bgColor = Preferences::instance()->getPreviewBgColor();
    else
      bgColor = Preferences::instance()->getViewerBgColor();
    m_bgColor[0] = (float)bgColor.r / 255.0f;
    m_bgColor[1] = (float)bgColor.g / 255.0f;
    m_bgColor[2] = (float)bgColor.b / 255.0f;
  }
  else {
    m_bgColor[0] = 0.0f;
    m_bgColor[1] = 0.0f;
    m_bgColor[2] = 0.0f;
  }
}

//-----------------------------------------------------------------------------

void OpenGLSceneViewer::setupPlacements() {
  TApp *app = TApp::instance();

  int frame = app->getCurrentFrame()->getFrame();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  // Camera affine
  TStageObjectId cameraId = xsh->getStageObjectTree()->getCurrentCameraId();
  TStageObject *camera = xsh->getStageObject(cameraId);
  TAffine cameraPlacement_t = camera->getPlacement(frame);
  double cameraZ = camera->getZ(frame);
  //m_cameraPlacementAff には view matrixは無いので注意！！
  //m_drawCameraAff =
  //  getViewQMatrix() * cameraPlacement * TScale((1000 + cameraZ) / 1000);
  m_cameraPlacementAff = OpenGLViewerDraw::toQMatrix(cameraPlacement_t * TScale((1000 + cameraZ) / 1000));

  // Table affine
  TStageObject *table = xsh->getStageObject(TStageObjectId::TableId);
  TAffine tablePlacement_t = table->getPlacement(frame);
  double tableZ = table->getZ(frame);
  TAffine placement;

  m_drawIsTableVisible = TStageObject::perspective(
    placement, cameraPlacement_t, cameraZ, tablePlacement_t, tableZ, 0);
  //m_tablePlacementAff には view matrixは無いので注意！！
  //m_drawTableAff = getViewQMatrix() * tablePlacement;
  m_tablePlacementAff = OpenGLViewerDraw::toQMatrix(tablePlacement_t);

  // Camera test check
  m_drawCameraTest = CameraTestCheck::instance()->isEnabled();

  if (m_previewMode == NO_PREVIEW) {
    m_drawEditingLevel = app->getCurrentFrame()->isEditingLevel();
    m_viewMode = m_drawEditingLevel ? LEVEL_VIEWMODE : SCENE_VIEWMODE;
    m_draw3DMode = is3DView() && (m_previewMode != SUBCAMERA_PREVIEW);
  }
  else {
    m_drawEditingLevel = false;
    m_viewMode = app->getCurrentFrame()->isEditingLevel() ? LEVEL_VIEWMODE : SCENE_VIEWMODE;
    m_draw3DMode = false;
  }

  //TODO:
  // Clip rect
  //if (!m_clipRect.isEmpty() && !m_draw3DMode) {
  //  m_actualClipRect = getActualClipRect(getViewQMatrix());
  //  m_actualClipRect += TPoint(width() * 0.5, height() * 0.5);
  //}

  TTool *tool = app->getCurrentTool()->getTool();
  //TODO:
  //if (tool && !m_isLocator) tool->setViewer(this);
}

//-----------------------------------------------------------------------------

void OpenGLSceneViewer::drawCameraStand() {
  TApp *app = TApp::instance();
  ToonzScene *scene = app->getCurrentScene()->getScene();
  //draw disk
  //if (!m_draw3DMode && viewTableToggle.getStatus() && m_drawIsTableVisible &&
  //   m_visualSettings.m_colorMask == 0 && m_drawEditingLevel == false &&
  //  !m_drawCameraTest) 
  {
    m_modelMatrix.push(m_modelMatrix.top());
    m_modelMatrix.top() *= m_tablePlacementAff;
    QMatrix4x4 MVPmatrix = m_projectionMatrix * getViewQMatrix() * m_modelMatrix.pop();
    OpenGLViewerDraw::instance()->setMVPMatrix(MVPmatrix);
    OpenGLViewerDraw::instance()->drawDisk();
  }

  // draw colorcard (with camera BG color)
  // Hide camera BG when level editing mode.
  //if (m_drawEditingLevel == false && viewClcToggle.getStatus() &&
  //  !m_drawCameraTest)
  {
    m_modelMatrix.push(m_modelMatrix.top());
    m_modelMatrix.top() *= m_cameraPlacementAff;
    QMatrix4x4 MVPmatrix = m_projectionMatrix * getViewQMatrix() * m_modelMatrix.pop();
    OpenGLViewerDraw::instance()->setMVPMatrix(MVPmatrix);
    OpenGLViewerDraw::instance()->drawColorcard(scene, m_visualSettings.m_colorMask);

  }

  // TODO Show white background when level editing mode.

  // TODO draw 3dframe

  // draw scene
  assert(glGetError() == GL_NO_ERROR);
  drawScene();
  assert((glGetError()) == GL_NO_ERROR);
}

//-----------------------------------------------------------------------------

void OpenGLSceneViewer::drawScene() {
  TApp *app = TApp::instance();
  ToonzScene *scene = app->getCurrentScene()->getScene();
  int frame = app->getCurrentFrame()->getFrame();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();
  TRect clipRect = getActualClipRect(getViewMatrix());
  clipRect += TPoint(width() * 0.5, height() * 0.5);

  // store VP matrix first. the model matrix will be computed for each raster 
  QMatrix4x4 VPmatrix = m_projectionMatrix * getViewQMatrix();
  OpenGLViewerDraw::instance()->setMVPMatrix(VPmatrix);
  OpenGLViewerDraw::instance()->setModelMatrix(m_modelMatrix.top());
  OpenGLViewerDraw::instance()->setViewportSize(size());
  /*TODO
  ChildStack *childStack = scene->getChildStack();
  bool editInPlace = editInPlaceToggle.getStatus() &&
    !app->getCurrentFrame()->isEditingLevel();

  bool fillFullColorRaster = TXshSimpleLevel::m_fillFullColorRaster;
  TXshSimpleLevel::m_fillFullColorRaster = false;
  */

  // Guided Drawing Check
  int useGuidedDrawing = Preferences::instance()->getGuidedDrawing();
  
  /*TODO
  m_minZ = 0;
  if (is3DView()) {
    Stage::OpenGlPainter painter(getViewMatrix(), clipRect, m_visualSettings,
      true, false);
    painter.enableCamera3D(true);
    painter.setPhi(m_phi3D);
    int xsheetLevel = 0;
    std::pair<TXsheet *, int> xr;
    if (editInPlace) {
      xr = childStack->getAncestor(frame);
      xsheetLevel = childStack->getAncestorCount();
    }
    else
      xr = std::make_pair(xsh, frame);

    Stage::VisitArgs args;
    args.m_scene = scene;
    args.m_xsh = xr.first;
    args.m_row = xr.second;
    args.m_col = app->getCurrentColumn()->getColumnIndex();
    OnionSkinMask osm = app->getCurrentOnionSkin()->getOnionSkinMask();
    args.m_osm = &osm;
    args.m_camera3d = true;
    args.m_xsheetLevel = xsheetLevel;
    args.m_currentFrameId =
      app->getCurrentXsheet()
      ->getXsheet()
      ->getCell(app->getCurrentFrame()->getFrame(), args.m_col)
      .getFrameId();
    args.m_isGuidedDrawingEnabled = useGuidedDrawing;

    // args.m_currentFrameId = app->getCurrentFrame()->getFid();
    Stage::visit(painter, args);

    m_minZ = painter.getMinZ();
  }
  else*/
  {
    // camera 2D (normale)
    TDimension viewerSize(width(), height());

    TAffine viewAff;// = getViewMatrix();

    /*TODO
    if (editInPlace) {
      TAffine aff;
      if (scene->getChildStack()->getAncestorAffine(aff, frame))
        viewAff = viewAff * aff.inv();
    }
    */

    //TODO
    //m_visualSettings.m_showBBox = viewBBoxToggle.getStatus();

    Stage::RasterPainter painter(viewerSize, viewAff, clipRect,
      m_visualSettings, true);

    // darken blended view mode for viewing the non-cleanuped and stacked
    // drawings
    painter.setRasterDarkenBlendedView(
      Preferences::instance()
      ->isShowRasterImagesDarkenBlendedInViewerEnabled());

    TFrameHandle *frameHandle = TApp::instance()->getCurrentFrame();
    if (app->getCurrentFrame()->isEditingLevel()) {
      Stage::visit(painter, app->getCurrentLevel()->getLevel(),
        app->getCurrentFrame()->getFid(),
        app->getCurrentOnionSkin()->getOnionSkinMask(),
        frameHandle->isPlaying(), useGuidedDrawing);
    }
    else {
      std::pair<TXsheet *, int> xr;
      int xsheetLevel = 0;
      /*TODO
      if (editInPlace) {
        xr = scene->getChildStack()->getAncestor(frame);
        xsheetLevel = scene->getChildStack()->getAncestorCount();
      }
      else*/
      xr = std::make_pair(xsh, frame);

      Stage::VisitArgs args;
      args.m_scene = scene;
      args.m_xsh = xr.first;
      args.m_row = xr.second;
      args.m_col = app->getCurrentColumn()->getColumnIndex();
      OnionSkinMask osm = app->getCurrentOnionSkin()->getOnionSkinMask();
      args.m_osm = &osm;
      args.m_xsheetLevel = xsheetLevel;
      args.m_isPlaying = frameHandle->isPlaying();
      args.m_currentFrameId =
        app->getCurrentXsheet()
        ->getXsheet()
        ->getCell(app->getCurrentFrame()->getFrame(), args.m_col)
        .getFrameId();
      args.m_isGuidedDrawingEnabled = useGuidedDrawing;
      args.m_isModern = true;
      Stage::visit(painter, args);
    }

    assert(glGetError() == 0);

    painter.openGLFlushRasterImages();
    /*
    QPoint pos;
    TAffine bboxAff;
    TRect bboxAffine;
    TRaster32P ras = painter.getFlushedRaster(pos, bboxAff, bboxAffine);

    if (ras.getPointer()) {
      myGlPushAttrib();
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE,
        GL_ONE_MINUS_SRC_ALPHA);  // The raster buffer is intended in
                                  // premultiplied form - thus the GL_ONE on src
      glDisable(GL_DEPTH_TEST);
      glDisable(GL_DITHER);
      glDisable(GL_LOGIC_OP);

      m_modelMatrix.push(QMatrix4x4());//identity matrix
      m_modelMatrix.top().translate(pos.x(), pos.y());
      m_modelMatrix.top().scale(ras->getLx(), ras->getLy());
      QMatrix4x4 MVPmatrix = m_projectionMatrix * getViewQMatrix() * m_modelMatrix.pop();

      OpenGLViewerDraw::instance()->drawSceneRaster(ras);

      ras->unlock();
      myGlPopAttrib();
    }
    */
    /*TODO
    TXshSimpleLevel::m_fillFullColorRaster = fillFullColorRaster;

    assert(glGetError() == 0);
    if (m_viewMode != LEVEL_VIEWMODE)
      drawSpline(getViewMatrix(), clipRect,
        m_referenceMode == CAMERA3D_REFERENCE, m_pixelSize);
    assert(glGetError() == 0);
    */
  }
}

//-----------------------------------------------------------------------------
/*! a factor for getting pixel-based zoom ratio
*/
double OpenGLSceneViewer::getDpiFactor() {
  // When the current unit is "pixels", always use a standard dpi
  if (Preferences::instance()->getPixelsOnly()) {
    return Stage::inch / Stage::standardDpi;
  }
  // When preview mode, use a camera DPI
  else if (isPreviewEnabled()) {
    return Stage::inch /
      TApp::instance()
      ->getCurrentScene()
      ->getScene()
      ->getCurrentCamera()
      ->getDpi()
      .x;
  }
  // When level editing mode, use an image DPI
  else if (TApp::instance()->getCurrentFrame()->isEditingLevel()) {
    TXshSimpleLevel *sl;
    sl = TApp::instance()->getCurrentLevel()->getSimpleLevel();
    if (!sl) return 1.;
    if (sl->getType() == PLI_XSHLEVEL) return 1.;
    if (sl->getImageDpi() != TPointD())
      return Stage::inch / sl->getImageDpi().x;
    if (sl->getDpi() != TPointD()) return Stage::inch / sl->getDpi().x;
    return 1.;
  }
  // When the special case in the scene editing mode:
  // If the option "ActualPixelViewOnSceneEditingMode" is ON,
  // use  current level's DPI set in the level settings.
  else if (Preferences::instance()
    ->isActualPixelViewOnSceneEditingModeEnabled() &&
    !CleanupPreviewCheck::instance()->isEnabled() &&
    !CameraTestCheck::instance()->isEnabled()) {
    TXshSimpleLevel *sl;
    sl = TApp::instance()->getCurrentLevel()->getSimpleLevel();
    if (!sl) return 1.;
    if (sl->getType() == PLI_XSHLEVEL) return 1.;
    if (sl->getDpi() == TPointD()) return 1.;
    // use default value for the argument of getDpi() (=TFrameId::NO_FRAME）
    // so that the dpi of the first frame in the level will be returned.
    return Stage::inch / sl->getDpi().x;
  }
  // When the scene editing mode without any option, don't think about DPI
  else {
    return 1.;
  }
}

//-----------------------------------------------------------------------------

TAffine OpenGLSceneViewer::getNormalZoomScale() {
  return TScale(getDpiFactor()).inv();
}

//=============================================================================
// EVENTS
//=============================================================================

void OpenGLSceneViewer::showEvent(QShowEvent *event) {
  TApp *app = TApp::instance();
  TSceneHandle *sceneHandle = app->getCurrentScene();

  //TODO : あとでStyleShortcutSwitchablePanelのshowEventでまとめる
  bool ret = connect(sceneHandle,
    SIGNAL(preferenceChanged(const QString &)), this,
    SLOT(onPreferenceChanged(const QString &)));
  onPreferenceChanged("");

  assert(ret);
}

//-----------------------------------------------------------------------------

void OpenGLSceneViewer::hideEvent(QHideEvent *event) {
  TApp *app = TApp::instance();
  TSceneHandle *sceneHandle = app->getCurrentScene();
  if (sceneHandle) sceneHandle->disconnect(this);
}

//=============================================================================
// SLOTS
//=============================================================================

void OpenGLSceneViewer::onPreferenceChanged(const QString& prefName) {
  if (prefName == "ViewerBgColor" || prefName == "PreviewBgColor" || prefName.isEmpty()) {
    updateBackgroundColor();
    update();
  }
}