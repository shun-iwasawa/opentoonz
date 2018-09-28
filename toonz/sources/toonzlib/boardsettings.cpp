#include "toonz/boardsettings.h"

// TnzLib includes
#include "toonz/toonzscene.h"
#include "toonz/tproject.h"
#include "toonz/sceneproperties.h"
#include "toutputproperties.h"
#include "tsystem.h"

#include <QImage>
#include <QDate>
#include <QDateTime>
#include <QFontMetricsF>

BoardItem::BoardItem() {

  m_name = "hogehoge";

  m_type = CurrentDateTime;

  m_rect = QRectF(0.1, 0.1, 0.8, 0.8);
  //m_rect = QRectF(0.2, 0.2, 0.6, 0.4);

  // 文字の最大サイズ
  m_maximumFontSize = 300;

  // 文字の色
  m_color = Qt::blue;

  // フォント
  m_font = QFont("Alial");

  //QString m_text;
}

QString BoardItem::getContentText(ToonzScene* scene) {
  switch (m_type) {
  case FreeText:         //自由テキスト (m_textに内容)
    return m_text;
    break;
  case ProjectName:      //プロジェクト名
    return scene->getProject()->getName().getQString();
    break;
  case SceneName:        //シーンファイル名
    return QString::fromStdWString(scene->getSceneName());
    break;
  case Duration_Frame:   //長さ（フレーム数）
    return QString::number(scene->getFrameCount());
    break;
  case Duration_SecFrame://長さ（秒＋コマ）
    {
      TOutputProperties *oprop = scene->getProperties()->getOutputProperties();
      int fps = (int)oprop->getFrameRate();
      int frame = scene->getFrameCount();
      return QString("%1 + %2").arg(QString::number(frame / fps), QString::number(frame % fps));
    }
    break;
  case Duration_HHMMSSFF://長さ（HH:MM:SS:FF）
    {
      TOutputProperties *oprop = scene->getProperties()->getOutputProperties();
      int fps = (int)oprop->getFrameRate();
      int frame = scene->getFrameCount();
      int hh = frame / (fps * 60 * 60);
      frame -= hh * fps * 60 * 60;
      int mm = frame / (fps * 60);
      frame -= mm * fps * 60;
      int ss = frame / fps;
      int ff = frame % fps;
      return QString::number(hh).rightJustified(2, '0') + ":" +
        QString::number(mm).rightJustified(2, '0') + ":" +
        QString::number(ss).rightJustified(2, '0') + ":" +
        QString::number(ff).rightJustified(2, '0');
    }
    break;
  case CurrentDate:      //現在の日（年 / 月 / 日）
    return QDate::currentDate().toString(Qt::DefaultLocaleLongDate);
    break;
  case CurrentDateTime:  //現在の日時（年 / 月 / 日 / 時 / 分 / 秒）
    return QDateTime::currentDateTime().toString(Qt::DefaultLocaleLongDate);
    break;
  case UserName:         //ユーザ名
    return TSystem::getUserName();
    break;
  case ScenePath_Aliased://シーンファイルパス（エイリアス付）
    return scene->codeFilePath(scene->getScenePath()).getQString();
    break;
  case ScenePath_Full:   //シーンファイルパス（フルパス）
    return scene->decodeFilePath(scene->getScenePath()).getQString();
    break;
  case MoviePath_Aliased://ムービーファイルパス（エイリアス付）
    {
      TOutputProperties *oprop = scene->getProperties()->getOutputProperties();
      return scene->codeFilePath(oprop->getPath()).getQString();
    }
    break;
  case MoviePath_Full:   //ムービーファイルパス（フルパス）
  {
    TOutputProperties *oprop = scene->getProperties()->getOutputProperties();
    return scene->decodeFilePath(oprop->getPath()).getQString();
  }
  break;
  }
  return QString();
}

//画像上のRectを返す
QRectF BoardItem::getItemRect(QSize imgSize) {
  QSizeF imgSizeF(imgSize);
  return QRectF(imgSizeF.width() * m_rect.left(), imgSizeF.height() * m_rect.top(),
    imgSizeF.width() * m_rect.width(), imgSizeF.height() * m_rect.height());
}

void BoardItem::drawItem(QPainter& p, QSize imgSize, int shrink, ToonzScene* scene) {
  QRectF itemRect = getItemRect(imgSize);

  if (m_type == Image) {
    QImage img(m_imgPath.getQString());
    float ratio = std::min((float)itemRect.width() / (float)img.width(), (float)itemRect.height() / (float)img.height());
    QSizeF imgSize((float)img.width() * ratio, (float)img.height() * ratio);
    QPointF imgTopLeft = itemRect.topLeft() + QPointF((itemRect.width() - imgSize.width())*0.5f, (itemRect.height() - imgSize.height())*0.5f);

    //画像描く
    p.drawImage(QRectF(imgTopLeft, imgSize), img);

    return;
  }

  QString contentText = getContentText(scene);

  QFont tmpFont(m_font);
  tmpFont.setPixelSize(100);
  QFontMetricsF tmpFm(tmpFont);
  QRectF tmpBounding = tmpFm.boundingRect(itemRect, Qt::AlignLeft | Qt::AlignTop, contentText);

  float ratio = std::min(itemRect.width() / tmpBounding.width(), itemRect.height() / tmpBounding.height());

  // ちょうど指定領域に入るフォントサイズを探索する
  int fontSize = (int)(100.0f * ratio);
  tmpFont.setPixelSize(fontSize);
  tmpFm = QFontMetricsF(tmpFont);
  tmpBounding = tmpFm.boundingRect(itemRect, Qt::AlignLeft | Qt::AlignTop, contentText);
  bool isInRect;
  if (itemRect.width() >= tmpBounding.width() && itemRect.height() >= tmpBounding.height())
    isInRect = true;
  else
    isInRect = false;
  while (1) {
    fontSize += (isInRect) ? 1 : -1;
    //std::cout << "font size = " << fontSize << std::endl;
    if (fontSize <= 0) // cannot draw 
      return;
    tmpFont.setPixelSize(fontSize);
    tmpFm = QFontMetricsF(tmpFont);
    tmpBounding = tmpFm.boundingRect(itemRect, Qt::AlignLeft | Qt::AlignTop, contentText);

    bool newIsInRect = (itemRect.width() >= tmpBounding.width() && itemRect.height() >= tmpBounding.height());
    if (isInRect != newIsInRect)
    {
      if (isInRect) fontSize--;
      break;
    }
  }

  //----
  fontSize = std::min(fontSize, m_maximumFontSize/shrink);

  QFont font(m_font);
  font.setPixelSize(fontSize);

  p.setFont(font);
  p.setPen(m_color);

  if (m_type == FreeText)
    p.drawText(itemRect, Qt::AlignLeft | Qt::AlignTop, contentText);
  else
    p.drawText(itemRect, Qt::AlignCenter, contentText);

}

//======================================================================================

BoardSettings::BoardSettings() {
  m_bgPath = TFilePath("D:\\OpenToonz 1.2 stuff\\library\\boards\\board1.png");

  m_items.push_back(BoardItem());
}

QImage BoardSettings::getBoardImage(TDimension& dim, int shrink, ToonzScene* scene) {
  QImage img(dim.lx, dim.ly, QImage::Format_ARGB32);

  QPainter painter(&img);

  painter.fillRect(img.rect(), Qt::gray);
  //draw background
  if (!m_bgPath.isEmpty())
    painter.drawImage(img.rect(), QImage(m_bgPath.getQString()));

  //draw each item
  for (int i = m_items.size() - 1; i >= 0; i--)
    m_items[i].drawItem(painter, img.rect().size(), shrink, scene);

  painter.end();

  return img;
}

TRaster32P BoardSettings::getBoardRaster(TDimension& dim, int shrink, ToonzScene* scene) {
  QImage img = getBoardImage(dim, shrink, scene);

  // convert QImage to TRaster 
  TRaster32P boardRas(dim);
  int img_y = img.height() - 1;
  for (int j = 0; j < dim.ly; j++, img_y--) {
    TPixel32 *pix = boardRas->pixels(j);
    QRgb *img_p = (QRgb *)img.scanLine(img_y);
    for (int i = 0; i < dim.lx; i++, pix++, img_p++) {
      (*pix).r = (typename TPixel32::Channel)(qRed(*img_p));
      (*pix).g = (typename TPixel32::Channel)(qGreen(*img_p));
      (*pix).b = (typename TPixel32::Channel)(qBlue(*img_p));
      (*pix).m = (typename TPixel32::Channel)(qAlpha(*img_p));
    }
  }
  return boardRas;
}

void BoardSettings::addNewItem(int insertAt) {
  m_items.insert(insertAt, BoardItem());
}

void BoardSettings::removeItem(int index) {
  if (index < 0 || index >= m_items.size()) return;
  m_items.removeAt(index);
}