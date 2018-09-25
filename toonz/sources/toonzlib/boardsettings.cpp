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

  m_type = CurrentDateTime;

  m_rect = QRectF(0.1, 0.1, 0.8, 0.8);
  //m_rect = QRectF(0.2, 0.2, 0.6, 0.4);

  // �����̍ő�T�C�Y
  //int m_maximumFontSize;

  // �����̐F
  //QColor m_color;

  // �t�H���g
  m_font = QFont("Alial");

  //QString m_text;
}

QString BoardItem::getContentText(ToonzScene* scene) {
  switch (m_type) {
  case FreeText:         //���R�e�L�X�g (m_text�ɓ��e)
    return m_text;
    break;
  case ProjectName:      //�v���W�F�N�g��
    return scene->getProject()->getName().getQString();
    break;
  case SceneName:        //�V�[���t�@�C����
    return QString::fromStdWString(scene->getSceneName());
    break;
  case Duration_Frame:   //�����i�t���[�����j
    return QString::number(scene->getFrameCount());
    break;
  case Duration_SecFrame://�����i�b�{�R�}�j
    {
      TOutputProperties *oprop = scene->getProperties()->getOutputProperties();
      int fps = (int)oprop->getFrameRate();
      int frame = scene->getFrameCount();
      return QString("%1 + %2").arg(QString::number(frame / fps), QString::number(frame % fps));
    }
    break;
  case Duration_HHMMSSFF://�����iHH:MM:SS:FF�j
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
  case CurrentDate:      //���݂̓��i�N / �� / ���j
    return QDate::currentDate().toString(Qt::DefaultLocaleLongDate);
    break;
  case CurrentDateTime:  //���݂̓����i�N / �� / �� / �� / �� / �b�j
    return QDateTime::currentDateTime().toString(Qt::DefaultLocaleLongDate);
    break;
  case UserName:         //���[�U��
    return TSystem::getUserName();
    break;
  case ScenePath_Aliased://�V�[���t�@�C���p�X�i�G�C���A�X�t�j
    return scene->codeFilePath(scene->getScenePath()).getQString();
    break;
  case ScenePath_Full:   //�V�[���t�@�C���p�X�i�t���p�X�j
    return scene->decodeFilePath(scene->getScenePath()).getQString();
    break;
  case MoviePath_Aliased://���[�r�[�t�@�C���p�X�i�G�C���A�X�t�j
    {
      TOutputProperties *oprop = scene->getProperties()->getOutputProperties();
      return scene->codeFilePath(oprop->getPath()).getQString();
    }
    break;
  case MoviePath_Full:   //���[�r�[�t�@�C���p�X�i�t���p�X�j
  {
    TOutputProperties *oprop = scene->getProperties()->getOutputProperties();
    return scene->decodeFilePath(oprop->getPath()).getQString();
  }
  break;
  }
  return QString();
}

//�摜���Rect��Ԃ�
QRectF BoardItem::getItemRect(QSize imgSize) {
  QSizeF imgSizeF(imgSize);
  return QRectF(imgSizeF.width() * m_rect.left(), imgSizeF.height() * m_rect.top(),
    imgSizeF.width() * m_rect.width(), imgSizeF.height() * m_rect.height());
}

void BoardItem::drawItem(QPainter& p, QSize imgSize, int shrink, ToonzScene* scene) {
  QRectF itemRect = getItemRect(imgSize);

  if (m_type == Image) {
    //�摜�`��
    return;
  }

  QString contentText = getContentText(scene);

  QFont tmpFont(m_font);
  tmpFont.setPixelSize(100);
  QFontMetricsF tmpFm(tmpFont);
  QRectF tmpBounding = tmpFm.boundingRect(itemRect, Qt::AlignLeft | Qt::AlignTop, contentText);

  float ratio = std::min(itemRect.width() / tmpBounding.width(), itemRect.height() / tmpBounding.height());

  // ���傤�ǎw��̈�ɓ���t�H���g�T�C�Y��T������
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
    std::cout << "font size = " << fontSize << std::endl;
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
  // �����ōő�t�H���g�T�C�Y�ŃN�����v����

  QFont font(m_font);
  font.setPixelSize(fontSize);

  p.setFont(font);
  p.setPen(Qt::red);

  p.drawText(itemRect, Qt::AlignLeft | Qt::AlignTop, contentText);

}

//======================================================================================

BoardSettings::BoardSettings() {
  m_bgPath = TFilePath("D:\\OpenToonz 1.2 stuff\\library\\boards\\board1.png");

  m_items.push_back(BoardItem());
}

TRaster32P BoardSettings::getBoardRaster(TDimension& dim, int shrink, ToonzScene* scene) {
  QImage img(dim.lx, dim.ly, QImage::Format_ARGB32);

  QPainter painter(&img);

  painter.fillRect(img.rect(), Qt::white);
  //draw background
  if(!m_bgPath.isEmpty())
    painter.drawImage(img.rect(), QImage(m_bgPath.getQString()));

  //draw each item
  for (int i = m_items.size() - 1; i >= 0; i--)
    m_items[i].drawItem(painter, img.rect().size(), shrink, scene);

  painter.end();

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