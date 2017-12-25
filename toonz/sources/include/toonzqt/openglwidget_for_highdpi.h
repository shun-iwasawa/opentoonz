#pragma once

#ifndef OPENGLWIDGET_FOR_HIGHDPI_H
#define OPENGLWIDGET_FOR_HIGHDPI_H

#include <QOpenGLWidget>
#include <QApplication>
#include <QDesktopWidget>
// use OpenGL 3.3 core profile
#include <QOpenGLFunctions_3_3_Core>
#include "toonzqt/gutil.h"


class OpenGLWidgetForHighDpi : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core {
public:
  OpenGLWidgetForHighDpi(QWidget *parent = Q_NULLPTR,
    Qt::WindowFlags f = Qt::WindowFlags())
    : QOpenGLWidget(parent, f) {}

  //  modify sizes for high DPI monitors
  int width() const { return QOpenGLWidget::width() * getDevPixRatio(); }
  int height() const { return QOpenGLWidget::height() * getDevPixRatio(); }
  QRect rect() const { return QRect(0, 0, width(), height()); }
};

#endif