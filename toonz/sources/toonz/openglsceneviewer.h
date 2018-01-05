#pragma once
#ifndef OPENGLSCENEVIEWER_H
#define OPENGLSCENEVIEWER_H

// TnzQt includes
#include "toonzqt/openglwidget_for_highdpi.h"

// TnzTools includes
#include "tools/tool.h"

// Qt includes
#include <QStack>
#include <QMatrix4x4>

//=============================================================================
// OpenGLSceneViewer
// new sceneviewer written in "modern" opengl way
//-----------------------------------------------------------------------------

class OpenGLSceneViewer final : public OpenGLWidgetForHighDpi, public TTool::Viewer {
  Q_OBJECT
public:
  enum PreviewMode { NO_PREVIEW = 0, FULL_PREVIEW = 1, SUBCAMERA_PREVIEW = 2 };
  // Zoom/Pan matrices are selected by ViewMode
  enum ViewMode { SCENE_VIEWMODE = 0, LEVEL_VIEWMODE = 1 };
 
  enum ReferenceMode {
    NORMAL_REFERENCE = 1,
    CAMERA3D_REFERENCE = 2,
    CAMERA_REFERENCE = 3,
    LEVEL_MODE = 128,
  };
private:

  // current view matrix 
  // (two different matrices for scene and level view)
  ViewMode m_viewMode = SCENE_VIEWMODE;
  QMatrix4x4 m_viewMatrix[2];
  
  // projection matrix
  QMatrix4x4 m_projectionMatrix;

  // model matrix
  QStack<QMatrix4x4> m_modelMatrix;
  
  // clear color = background color
  GLfloat m_bgColor[3] = { 0.0f, 0.0f, 0.0f };

  PreviewMode m_previewMode = NO_PREVIEW;
  ReferenceMode m_referenceMode = NORMAL_REFERENCE;

  // placement of objects
  QMatrix4x4 m_cameraPlacementAff;
  QMatrix4x4 m_tablePlacementAff;
  bool m_drawIsTableVisible;
  bool m_drawCameraTest;
  bool m_drawEditingLevel;

  // 3D
  bool m_draw3DMode;

  double m_pixelSize = 1.0;
  TPointD m_dpiScale = TPointD(1, 1);
  QString m_toolDisableReason = "";
  // for partial GL updating
  TRectD m_clipRect;

  bool m_isLocator = false;
public:

  OpenGLSceneViewer(QWidget *parent);
  ~OpenGLSceneViewer();

  bool isPreviewEnabled() const { return m_previewMode != NO_PREVIEW; }
  
  // overriding TTool::Viewer
  double getPixelSize() const override { return m_pixelSize; }
  void invalidateAll() override { update(); }//TODO
  void GLInvalidateAll() override { update(); }//TODO
  void GLInvalidateRect(const TRectD &rect) override { update(); }//TODO
  void invalidateToolStatus() override;
  TAffine getViewMatrix() const override;
  QMatrix4x4 getViewQMatrix() const override;
  int posToColumnIndex(const TPoint &p, double distance, //TODO
    bool includeInvisible = true) const override {
    return 0;
  }
  void posToColumnIndexes(const TPoint &p, std::vector<int> &indexes,
    double distance, bool includeInvisible = true) const override {}//TODO
  int posToRow(const TPoint &p, double distance, //TODO
    bool includeInvisible = true) const override {
    return 0;
  }
  TPoint worldToPos(const TPointD &worldPos) const override;
  int pick(const TPoint &point) override { return 0; }//TODO
  TPointD winToWorld(const TPoint &winPos) const override { return TPointD(); }//TODO
  void pan(const TPoint &delta) override {}//TODO
  void zoom(const TPointD &center, double scaleFactor) override {}//TODO
  void rotate(const TPointD &center, double angle) override {}//TODO
  void rotate3D(double dPhi, double dTheta) override {}//TODO
  bool is3DView() const override;//ok
  bool getIsFlippedX() const override { return false; }//TODO
  bool getIsFlippedY() const override { return false; }//TODO
  double projectToZ(const TPoint &delta) override { return 0.0; }//TODO
  TPointD getDpiScale() const override { return m_dpiScale; }
  int getVGuideCount() override { return 0; }//TODO
  int getHGuideCount() override { return 0; }//TODO
  double getHGuide(int index) override { return 0; }//TODO
  double getVGuide(int index) override { return 0; }//TODO
  void resetInputMethod() override {}//TODO
  void setFocus() override {} //TODO
  TRectD getGeometry() const override { return TRectD(); }//TODO
  




                                                          // a factor for getting pixel-based zoom ratio
  double getDpiFactor();

  TAffine getNormalZoomScale();
protected:

  void initializeGL() override;
  void resizeGL(int width, int height) override;
  void paintGL() override;

  // Paint methods
  void updateBackgroundColor();
  void setupPlacements(); // similar to SceneViewer::drawBuildVars
  void drawCameraStand();
  void drawScene();
  TRect getActualClipRect(const TAffine &aff);

  void showEvent(QShowEvent *event);
  void hideEvent(QHideEvent *event);

protected slots:
  void onPreferenceChanged(const QString& prefName);
};

#endif