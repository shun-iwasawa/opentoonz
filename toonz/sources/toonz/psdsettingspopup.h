#pragma once

#ifndef PSDSETTINGSPOPUP_H
#define PSDSETTINGSPOPUP_H

#include "toonzqt/dvdialog.h"
#include "tfilepath.h"
#include "../common/psdlib/psd.h"

#include <QButtonGroup>
#include <QTextEdit>
//#include <QTreeWidget>

// forward declaration
class QPushButton;
class QComboBox;
class QStackedLayout;

namespace DVGui {
class CheckBox;
}

//=============================================================================
// PsdSettingsPopup
//-----------------------------------------------------------------------------

class PsdSettingsPopup final : public DVGui::Dialog {
  Q_OBJECT
  // Loading Mode
  // FLAT: psd flat image
  // FRAMES: all psd layers are frames of a single Tlevel
  // COLUMNS: each psd layer is a TLevel with only one frame.
  // FOLDER: each psd layer is a TLevel and
  //				 each psd folder is a TLevel where each psd
  // layer
  // contained
  // into folder is a frame of TLevel
public:
  enum Mode { FLAT, FRAMES, FRAMES_GROUP, COLUMNS, FOLDER };

  enum GroupOptions {
    IgnoreGroups,
    GroupAsSubXsheet,
    GroupAsColumn,
    GroupAsSingleImage
  };
private:

  TFilePath m_path;
  std::vector<TFilePath> m_psdLevelPaths;
  std::vector<int> loadedLevelId;

  Mode m_mode;
  QPushButton *m_okBtn;
  QPushButton *m_cancelBtn;
  QComboBox *m_loadMode, *m_levelNameType;
  DVGui::CheckBox *m_createSubXSheet;
  
  QStackedLayout* m_groupOptionStack;
  QComboBox *m_frameGroupOptions;
  QComboBox *m_columnsGroupOptions;

  DVGui::CheckBox *m_skipInvisible, *m_skipBackground;

  // QTreeWidget *m_psdTree;	// per adesso non serve. Servirà in un secondo
  // momento quando implemento la scelta dei livelli
  // da caricare
  QTextEdit *m_modeDescription;
  TPSDParser *m_psdparser;
  QLabel *m_filename;   // Name
  QLabel *m_parentDir;  // Path
  
public slots:
  void onOk();

public:
  PsdSettingsPopup();

  void setPath(const TFilePath &path);
  int getPsdLevelCount() { return m_psdLevelPaths.size(); };
  TFilePath getPsdPath(int levelIndex);
  TFilePath getPsdFramePath(int levelIndex, int frameIndex);
  int getFramesCount(int levelIndex);
  bool isFolder(int levelIndex);
  bool isSubFolder(int levelIndex, int frameIndex);
  bool subxsheet();
  int levelNameType();

  int getGroupOption();
  int getSubfolderLevelIndex(int psdLevelIndex, int frameIndex);

private:
  void doPsdParser();

protected slots:
  void onModeChanged();
  void onGroupOptionChange();
};

#endif  // PsdSettingsPopup_H
