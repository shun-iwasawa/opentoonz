#pragma once

#ifndef OPENGL_VIEWERDRAW_INCLUDED
#define OPENGL_VIEWERDRAW_INCLUDED

//=============================================================================
// OpenGLViewerDraw
// "modern" version of ViewerDraw
// shader programs and vbo are shared among viewers
//-----------------------------------------------------------------------------

#include "tgeometry.h"

#include <QOpenGLBuffer>
#include <QMatrix4x4>

class QOpenGLShader;
class QOpenGLShaderProgram;

class OpenGLViewerDraw { //singleton
  struct SimpleShader{
    // shader for simple objects
    QOpenGLShader* vert = nullptr;
    QOpenGLShader* frag = nullptr;
    // shader program for simple objects
    QOpenGLShaderProgram* program = nullptr;
    int mvpMatrixUniform = -1;
    int colorUniform = -1;
    int vertexAttrib = -1;
  }m_simpleShader;
  //disk vbo
  QOpenGLBuffer m_diskVBO;
  int m_diskVertexOffset[2];

  OpenGLViewerDraw() {};
  ~OpenGLViewerDraw();

public:
  static OpenGLViewerDraw *instance();

  // called once and create shader programs
  void initialize();
  // called once in main() on the end
  void finalize();

  void drawDisk(QMatrix4x4& mvp);
  void createDiskVBO();

  void drawColorcard(UCHAR channel, QMatrix4x4& mvp);
  TRectD getCameraRect();

  static QMatrix4x4& toQMatrix(const TAffine&aff);
  static TAffine& toTAffine(const QMatrix4x4&matrix);
};

#endif
