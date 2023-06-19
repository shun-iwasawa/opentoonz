

#include "xshcellviewer.h"

// Tnz6 includes
#include "xsheetviewer.h"
#include "tapp.h"
#include "menubarcommandids.h"
#include "cellselection.h"
#include "keyframeselection.h"
#include "filmstripselection.h"
#include "filmstripcommand.h"
#include "xsheetdragtool.h"
#include "iocommand.h"
#include "castselection.h"
#include "cellkeyframeselection.h"
#include "keyframedata.h"
#include "columnselection.h"
#include "orientation.h"

// TnzTools includes
#include "tools/cursormanager.h"
#include "tools/toolhandle.h"

// TnzQt includes
#include "toonzqt/gutil.h"
#include "toonzqt/tselectionhandle.h"
#include "historytypes.h"
#include "toonzqt/menubarcommand.h"

// TnzLib includes
#include "toonz/tscenehandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/txshlevelhandle.h"
#include "toonz/tobjecthandle.h"
#include "toonz/tframehandle.h"
#include "toonz/txshleveltypes.h"
#include "toonz/txsheet.h"
#include "toonz/toonzscene.h"
#include "toonz/txshsimplelevel.h"
#include "toonz/txshsoundcolumn.h"
#include "toonz/txshcell.h"
#include "toonz/tstageobject.h"
#include "toonz/tstageobjecttree.h"
#include "toonz/sceneproperties.h"
#include "toonz/columnfan.h"
#include "toonz/levelset.h"
#include "toonz/levelproperties.h"
#include "toonz/txshsoundlevel.h"
#include "toonz/txshnoteset.h"
#include "toonz/txshcolumn.h"
#include "toonz/tcolumnfx.h"
#include "toonz/txshchildlevel.h"
#include "toonz/txshzeraryfxcolumn.h"
#include "toonz/txshsoundtextcolumn.h"
#include "toonz/txshsoundtextlevel.h"
#include "toonz/txshpalettelevel.h"
#include "toonz/doubleparamcmd.h"
#include "toonz/preferences.h"
#include "toutputproperties.h"

// TnzBase includes
#include "tdoublekeyframe.h"

// TnzCore includes
#include "tconvert.h"

// Qt includes
#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QUrl>
#include <QToolTip>
#include <QApplication>
#include <QClipboard>

namespace {
const QString CMenu_OpenMemo         = "CMenu_OpenMemo";
const QString CMenu_DeleteMemo       = "CMenu_DeleteMemo";
const QString CMenu_Interp1s         = "CMenu_Interp1s";
const QString CMenu_Interp2s         = "CMenu_Interp2s";
const QString CMenu_Interp3s         = "CMenu_Interp3s";
const QString CMenu_Interp4s         = "CMenu_Interp4s";
const QString CMenu_ReplaceLevelWith = "CMenu_ReplaceLevelWith";
const QString CMenu_CellMark         = "CMenu_CellMark";

const QString CMenuType_Note    = "Note";
const QString CMenuType_Key     = "Key";
const QString CMenuType_KeyLine = "KeyLine";
const QString CMenuType_Cell    = "Cell";

const bool checkContainsSingleLevel(TXshColumn *column,
                                    const std::string &columnName) {
  TXshLevel *level           = nullptr;
  TXshCellColumn *cellColumn = column->getCellColumn();
  if (cellColumn) {
    int i, r0, r1;
    cellColumn->getRange(r0, r1);
    for (i = r0; i <= r1; i++) {
      TXshCell cell = cellColumn->getCell(i);
      if (cell.isEmpty()) continue;
      TXshLevel *lvl = cell.m_level.getPointer();
      if (!level)
        level = lvl;
      else if (lvl != level)
        return false;
    }
    // column name should be unspecified or same as the content level
    return level != nullptr &&
           (columnName.empty() || level->getName() == to_wstring(columnName));
  }
  return false;
}

bool selectionContainTlvImage(TCellSelection *selection, TXsheet *xsheet,
                              bool onlyTlv = false) {
  int r0, r1, c0, c1;
  selection->getSelectedCells(r0, c0, r1, c1);

  // Verifico se almeno un livello della selezione e' un tlv
  int c, r;
  for (c = c0; c <= c1; c++)
    for (r = r0; r <= r1; r++) {
      TXshCell cell = xsheet->getCell(r, c);
      TXshSimpleLevel *level =
          cell.isEmpty() ? (TXshSimpleLevel *)0 : cell.getSimpleLevel();
      if (!level) continue;

      if (level->getType() != TZP_XSHLEVEL && onlyTlv) return false;
      if (level->getType() == TZP_XSHLEVEL && !onlyTlv) return true;
    }

  return false;
}

//-----------------------------------------------------------------------------

bool selectionContainRasterImage(TCellSelection *selection, TXsheet *xsheet,
                                 bool onlyRaster = false) {
  int r0, r1, c0, c1;
  selection->getSelectedCells(r0, c0, r1, c1);

  // Verifico se almeno un livello della selezione e' un tlv
  int c, r;
  for (c = c0; c <= c1; c++)
    for (r = r0; r <= r1; r++) {
      TXshCell cell = xsheet->getCell(r, c);
      TXshSimpleLevel *level =
          cell.isEmpty() ? (TXshSimpleLevel *)0 : cell.getSimpleLevel();
      if (!level) continue;

      if (level->getType() != OVL_XSHLEVEL && onlyRaster) return false;
      if (level->getType() == OVL_XSHLEVEL && !onlyRaster) return true;
    }

  return false;
}

//-----------------------------------------------------------------------------

bool selectionContainLevelImage(TCellSelection *selection, TXsheet *xsheet) {
  int r0, r1, c0, c1;
  selection->getSelectedCells(r0, c0, r1, c1);
  // Verifico se almeno un livello della selezione e' un tlv, un pli o un
  // fullcolor (!= .psd)
  int c, r;
  for (c = c0; c <= c1; c++)
    for (r = r0; r <= r1; r++) {
      TXshCell cell = xsheet->getCell(r, c);
      TXshSimpleLevel *level =
          cell.isEmpty() ? (TXshSimpleLevel *)0 : cell.getSimpleLevel();
      if (!level) continue;

      std::string ext = level->getPath().getType();
      int type        = level->getType();
      if (type == TZP_XSHLEVEL || type == PLI_XSHLEVEL || type == OVL_XSHLEVEL)
        return true;
    }

  return false;
}

//-----------------------------------------------------------------------------
/*! convert the last one digit of the frame number to alphabet
    Ex.  12 -> 1B    21 -> 2A   30 -> 3
*/
void parse_with_letter(const QString &text, std::wstring &levelName,
                       TFrameId &fid) {
  QRegExp spaces("\\t|\\s");
  QRegExp numbers("\\d+");
  QRegExp characters("[^\\d+]");
  QString str = text;

  // remove final spaces
  int size = str.size();
  while (str.lastIndexOf(spaces) == size - 1 && size > 0)
    str.remove(str.size() - 1, 1);
  if (str.isEmpty()) {
    levelName = L"";
    fid       = TFrameId::NO_FRAME;
    return;
  }

  int lastSpaceIndex = str.lastIndexOf(spaces);
  // if input only digits / alphabet
  if (lastSpaceIndex == -1) {
    // in case of only level name
    if (str.contains(characters) && !str.contains(numbers)) {
      levelName = text.toStdWString();
      fid       = TFrameId::NO_FRAME;
    }
    // in case of input frame number + alphabet
    else if (str.contains(numbers) && str.contains(characters)) {
      levelName = L"";

      QString appendix = str.right(1);

      int appendNum = 0;

      if (appendix == QString('A') || appendix == QString('a'))
        appendNum = 1;
      else if (appendix == QString('B') || appendix == QString('b'))
        appendNum = 2;
      else if (appendix == QString('C') || appendix == QString('c'))
        appendNum = 3;
      else if (appendix == QString('D') || appendix == QString('d'))
        appendNum = 4;
      else if (appendix == QString('E') || appendix == QString('e'))
        appendNum = 5;
      else if (appendix == QString('F') || appendix == QString('f'))
        appendNum = 6;
      else if (appendix == QString('G') || appendix == QString('g'))
        appendNum = 7;
      else if (appendix == QString('H') || appendix == QString('h'))
        appendNum = 8;
      else if (appendix == QString('I') || appendix == QString('i'))
        appendNum = 9;

      str.chop(1);

      if (str.contains(characters) || !str.contains(numbers)) {
        levelName = text.toStdWString();
        fid       = TFrameId::NO_FRAME;
        return;
      }

      fid = TFrameId(str.toInt() * 10 + appendNum);
    }
    // in case of input only frame number
    else if (str.contains(numbers)) {
      levelName = L"";
      fid       = TFrameId(str.toInt() * 10);
    }
  }
  // if input both the level name and frame number
  else {
    QString lastString = str.right(str.size() - 1 - lastSpaceIndex);
    if (lastString.contains(numbers)) {
      // level name
      QString firstString = str.left(lastSpaceIndex);
      levelName           = firstString.toStdWString();

      // frame number + alphabet
      if (lastString.contains(characters)) {
        QString appendix = lastString.right(1);

        int appendNum = 0;

        if (appendix == QString('A') || appendix == QString('a'))
          appendNum = 1;
        else if (appendix == QString('B') || appendix == QString('b'))
          appendNum = 2;
        else if (appendix == QString('C') || appendix == QString('c'))
          appendNum = 3;
        else if (appendix == QString('D') || appendix == QString('d'))
          appendNum = 4;
        else if (appendix == QString('E') || appendix == QString('e'))
          appendNum = 5;
        else if (appendix == QString('F') || appendix == QString('f'))
          appendNum = 6;
        else if (appendix == QString('G') || appendix == QString('g'))
          appendNum = 7;
        else if (appendix == QString('H') || appendix == QString('h'))
          appendNum = 8;
        else if (appendix == QString('I') || appendix == QString('i'))
          appendNum = 9;

        lastString.chop(1);

        if (lastString.contains(characters) || !lastString.contains(numbers)) {
          levelName = text.toStdWString();
          fid       = TFrameId::NO_FRAME;
          return;
        }

        fid = TFrameId(lastString.toInt() * 10 + appendNum);
      }
      // only frame number
      else if (lastString.contains(numbers)) {
        fid = TFrameId(lastString.toInt() * 10);
      }

    } else {
      levelName = text.toStdWString();
      fid       = TFrameId::NO_FRAME;
    }
  }
}

//-----------------------------------------------------------------------------

void parse(const QString &text, std::wstring &levelName, TFrameId &fid) {
  QRegExp spaces("\\t|\\s");
  QRegExp numbers("\\d+");
  QRegExp characters("[^\\d+]");
  QRegExp fidWithSuffix(TFilePath::fidRegExpStr());
  // QRegExp fidWithSuffix("([0-9]+)([a-z]?)");
  QString str = text;

  // remove final spaces
  int size = str.size();
  while (str.lastIndexOf(spaces) == size - 1 && size > 0)
    str.remove(str.size() - 1, 1);
  if (str.isEmpty()) {
    levelName = L"";
    fid       = TFrameId::NO_FRAME;
    return;
  }
  int lastSpaceIndex = str.lastIndexOf(spaces);
  if (lastSpaceIndex == -1) {
    if (str.contains(numbers) && !str.contains(characters)) {
      levelName = L"";
      fid       = TFrameId(str.toInt());
    } else if (fidWithSuffix.exactMatch(str)) {
      levelName = L"";
      fid       = TFrameId(fidWithSuffix.cap(1).toInt(), fidWithSuffix.cap(2));
      // fidWithSuffix.cap(2) == "" ? 0 : fidWithSuffix.cap(2).toLatin1()[0]);
    } else if (str.contains(characters)) {
      levelName = text.toStdWString();
      fid       = TFrameId::NO_FRAME;
    }
  } else {
    QString lastString = str.right(str.size() - 1 - lastSpaceIndex);
    if (lastString.contains(numbers) && !lastString.contains(characters)) {
      QString firstString = str.left(lastSpaceIndex);
      levelName           = firstString.toStdWString();
      fid                 = TFrameId(lastString.toInt());
    } else if (fidWithSuffix.exactMatch(lastString)) {
      QString firstString = str.left(lastSpaceIndex);
      levelName           = firstString.toStdWString();
      fid = TFrameId(fidWithSuffix.cap(1).toInt(), fidWithSuffix.cap(2));
      // fidWithSuffix.cap(2) == "" ? 0 : fidWithSuffix.cap(2).toLatin1()[0]);
    } else if (lastString.contains(characters)) {
      levelName = text.toStdWString();
      fid       = TFrameId::NO_FRAME;
    }
  }
}

//-----------------------------------------------------------------------------

bool isGlobalKeyFrameWithSameTypeDiffFromLinear(TStageObject *stageObject,
                                                int frame) {
  if (!stageObject->isFullKeyframe(frame)) return false;
  TDoubleKeyframe::Type type =
      stageObject->getParam(TStageObject::T_Angle)->getKeyframeAt(frame).m_type;
  if (type == TDoubleKeyframe::Linear) return false;
  if (type != stageObject->getParam(TStageObject::T_X)
                  ->getKeyframeAt(frame)
                  .m_type ||
      type != stageObject->getParam(TStageObject::T_Y)
                  ->getKeyframeAt(frame)
                  .m_type ||
      type != stageObject->getParam(TStageObject::T_Z)
                  ->getKeyframeAt(frame)
                  .m_type ||
      type != stageObject->getParam(TStageObject::T_SO)
                  ->getKeyframeAt(frame)
                  .m_type ||
      type != stageObject->getParam(TStageObject::T_ScaleX)
                  ->getKeyframeAt(frame)
                  .m_type ||
      type != stageObject->getParam(TStageObject::T_ScaleY)
                  ->getKeyframeAt(frame)
                  .m_type ||
      type != stageObject->getParam(TStageObject::T_Scale)
                  ->getKeyframeAt(frame)
                  .m_type ||
      type != stageObject->getParam(TStageObject::T_Path)
                  ->getKeyframeAt(frame)
                  .m_type ||
      type != stageObject->getParam(TStageObject::T_ShearX)
                  ->getKeyframeAt(frame)
                  .m_type ||
      type != stageObject->getParam(TStageObject::T_ShearY)
                  ->getKeyframeAt(frame)
                  .m_type)
    return false;
  return true;
}

//-----------------------------------------------------------------------------

bool rectContainsPos(QList<QRect> rects, QPoint pos) {
  int i;
  for (i = 0; i < rects.size(); i++)
    if (rects.at(i).contains(pos)) return true;
  return false;
}

//-----------------------------------------------------------------------------

bool isGlobalKeyFrameWithSamePrevTypeDiffFromLinear(TStageObject *stageObject,
                                                    int frame) {
  if (!stageObject->isFullKeyframe(frame)) return false;
  TDoubleKeyframe::Type type = stageObject->getParam(TStageObject::T_Angle)
                                   ->getKeyframeAt(frame)
                                   .m_prevType;
  if (type == TDoubleKeyframe::Linear) return false;
  if (type != stageObject->getParam(TStageObject::T_X)
                  ->getKeyframeAt(frame)
                  .m_prevType ||
      type != stageObject->getParam(TStageObject::T_Y)
                  ->getKeyframeAt(frame)
                  .m_prevType ||
      type != stageObject->getParam(TStageObject::T_Z)
                  ->getKeyframeAt(frame)
                  .m_prevType ||
      type != stageObject->getParam(TStageObject::T_SO)
                  ->getKeyframeAt(frame)
                  .m_prevType ||
      type != stageObject->getParam(TStageObject::T_ScaleX)
                  ->getKeyframeAt(frame)
                  .m_prevType ||
      type != stageObject->getParam(TStageObject::T_ScaleY)
                  ->getKeyframeAt(frame)
                  .m_prevType ||
      type != stageObject->getParam(TStageObject::T_Scale)
                  ->getKeyframeAt(frame)
                  .m_prevType ||
      type != stageObject->getParam(TStageObject::T_Path)
                  ->getKeyframeAt(frame)
                  .m_prevType ||
      type != stageObject->getParam(TStageObject::T_ShearX)
                  ->getKeyframeAt(frame)
                  .m_prevType ||
      type != stageObject->getParam(TStageObject::T_ShearY)
                  ->getKeyframeAt(frame)
                  .m_prevType)
    return false;
  return true;
}

//-----------------------------------------------------------------------------

int getParamStep(TStageObject *stageObject, int frame) {
  // this is a pretty useless function outside of global keyframes.
  TDoubleKeyframe keyFrame =
      stageObject->getParam(TStageObject::T_Angle)->getKeyframeAt(frame);
  return keyFrame.m_step;
}

//-----------------------------------------------------------------------------

QIcon getColorChipIcon(TPixel32 color) {
  QPixmap pm(15, 15);
  pm.fill(QColor(color.r, color.g, color.b));
  return QIcon(pm);
}

//-----------------------------------------------------------------------------

void setParamStep(int indexKeyframe, int step, TDoubleParam *param) {
  KeyframeSetter setter(param, indexKeyframe);
  setter.setStep(step);
}

//=============================================================================
// RenameCellUndo
//-----------------------------------------------------------------------------

class RenameCellUndo final : public TUndo {
  int m_row;
  int m_col;
  const TXshCell m_oldCell;
  const TXshCell m_newCell;

public:
  // indices sono le colonne inserite
  RenameCellUndo(int row, int col, TXshCell oldCell, TXshCell newCell)
      : m_row(row), m_col(col), m_oldCell(oldCell), m_newCell(newCell) {}

  void setcell(const TXshCell cell) const {
    TApp *app    = TApp::instance();
    TXsheet *xsh = app->getCurrentXsheet()->getXsheet();
    assert(xsh);
    if (cell.isEmpty())
      xsh->clearCells(m_row, m_col);
    else
      xsh->setCell(m_row, m_col, cell);
    app->getCurrentXsheet()->notifyXsheetChanged();
  }

  void undo() const override { setcell(m_oldCell); }

  void redo() const override { setcell(m_newCell); }

  int getSize() const override { return sizeof *this; }

  QString getHistoryString() override {
    return QObject::tr("Rename Cell  at Column %1  Frame %2")
        .arg(QString::number(m_col + 1))
        .arg(QString::number(m_row + 1));
  }
  int getHistoryType() override { return HistoryType::Xsheet; }
};

//-----------------------------------------------------------------------------

class RenameTextCellUndo final : public TUndo {
  int m_row;
  int m_col;
  const TXshCell m_oldCell;
  const TXshCell m_newCell;
  QString m_oldText, m_newText;
  TXshSoundTextLevel *m_level;

public:
  RenameTextCellUndo(int row, int col, TXshCell oldCell, TXshCell newCell,
                     QString oldText, QString newText,
                     TXshSoundTextLevel *level)
      : m_row(row)
      , m_col(col)
      , m_oldCell(oldCell)
      , m_newCell(newCell)
      , m_oldText(oldText)
      , m_newText(newText)
      , m_level(level) {}

  void setcell(const TXshCell cell, QString text = "") const {
    TApp *app    = TApp::instance();
    TXsheet *xsh = app->getCurrentXsheet()->getXsheet();
    assert(xsh);
    if (cell.isEmpty())
      xsh->clearCells(m_row, m_col);
    else {
      xsh->setCell(m_row, m_col, cell);
      m_level->setFrameText(cell.getFrameId().getNumber() - 1, text);
    }
    app->getCurrentXsheet()->notifyXsheetChanged();
  }

  void undo() const override { setcell(m_oldCell, m_oldText); }

  void redo() const override { setcell(m_newCell, m_newText); }

  int getSize() const override { return sizeof *this; }

  QString getHistoryString() override {
    return QObject::tr("Change Text at Column %1  Frame %2")
        .arg(QString::number(m_col + 1))
        .arg(QString::number(m_row + 1));
  }
  int getHistoryType() override { return HistoryType::Xsheet; }
};

// display upper-directional smart tab only when pressing ctrl key
bool isCtrlPressed = false;

//-----------------------------------------------------------------------------
}  // namespace
//-----------------------------------------------------------------------------

namespace XsheetGUI {

//=============================================================================
// SetCellMarkUndo
//-----------------------------------------------------------------------------

SetCellMarkUndo::SetCellMarkUndo(int row, int col, int idAfter)
    : m_row(row), m_col(col), m_idAfter(idAfter) {
  TXshCellColumn *cellColumn = TApp::instance()
                                   ->getCurrentXsheet()
                                   ->getXsheet()
                                   ->getColumn(col)
                                   ->getCellColumn();
  assert(cellColumn);
  m_idBefore = cellColumn->getCellMark(row);
  if (m_idBefore == m_idAfter) m_idAfter = -1;
}

void SetCellMarkUndo::setId(int id) const {
  TXshCellColumn *cellColumn = TApp::instance()
                                   ->getCurrentXsheet()
                                   ->getXsheet()
                                   ->getColumn(m_col)
                                   ->getCellColumn();
  assert(cellColumn);
  cellColumn->setCellMark(m_row, id);
  TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();
}

void SetCellMarkUndo::undo() const { setId(m_idBefore); }

void SetCellMarkUndo::redo() const { setId(m_idAfter); }

int SetCellMarkUndo::getSize() const { return sizeof *this; }

QString SetCellMarkUndo::getHistoryString() {
  QString markName;
  if (m_idAfter < 0)
    markName = QObject::tr("None", "Cell Mark");
  else
    markName = TApp::instance()
                   ->getCurrentScene()
                   ->getScene()
                   ->getProperties()
                   ->getCellMark(m_idAfter)
                   .name;
  return QObject::tr("Set Cell Mark at Column %1  Frame %2 to %3")
      .arg(QString::number(m_col + 1))
      .arg(QString::number(m_row + 1))
      .arg(markName);
}
int SetCellMarkUndo::getHistoryType() { return HistoryType::Xsheet; }

//=============================================================================
// RenameCellField
//-----------------------------------------------------------------------------

RenameCellField::RenameCellField(QWidget *parent, XsheetViewer *viewer)
    : QLineEdit(parent), m_viewer(viewer), m_row(-1), m_col(-1) {
  connect(this, SIGNAL(returnPressed()), SLOT(onReturnPressed()));
  setContextMenuPolicy(Qt::PreventContextMenu);
  setObjectName("RenameCellField");
  setAttribute(Qt::WA_TranslucentBackground, true);
  installEventFilter(this);
}

//-----------------------------------------------------------------------------

void RenameCellField::paintEvent(QPaintEvent *e) {
  // use paintEvent() of base class to do the main work
  QLineEdit::paintEvent(e);
  // draw cursor (if widget has focus)
  if (hasFocus() &&
      m_viewer->getRenameCellFieldCursorOverlapColor().alpha() > 0) {
    QRect cRect = cursorRect();
    double dw   = (double)(cRect.width() - 1) / 2.;
    cRect.adjust((int)std::ceil(dw), 0, -(int)std::floor(dw), 0);
    QPainter p(this);
    p.fillRect(cRect, m_viewer->getRenameCellFieldCursorOverlapColor());
  }
}

//-----------------------------------------------------------------------------

void RenameCellField::showInRowCol(int row, int col, bool multiColumnSelected) {
  const Orientation *o = m_viewer->orientation();

  m_viewer->scrollTo(row, col);

  m_row = row;
  m_col = col;

  QString fontName = Preferences::instance()->getInterfaceFont();
  if (fontName == "") {
#ifdef _WIN32
    fontName = "Arial";
#else
    fontName = "Helvetica";
#endif
  }
  static QFont font(fontName, -1, QFont::Normal);
  font.setPixelSize(XSHEET_FONT_PX_SIZE);
  setFont(font);
  setAlignment(Qt::AlignRight | Qt::AlignBottom);

  // Se la cella non e' vuota setto il testo
  TXsheet *xsh = m_viewer->getXsheet();

  // adjust text position
  int padding = 3;
  if (Preferences::instance()->isShowKeyframesOnXsheetCellAreaEnabled()) {
    TStageObject *pegbar = xsh->getStageObject(m_viewer->getObjectId(col));
    int r0, r1;
    if (pegbar && pegbar->getKeyframeRange(r0, r1)) padding += 9;
  }

  TXshCell cell = xsh->getCell(row, col);
  QPoint xy     = m_viewer->positionToXY(CellPosition(row, col)) - QPoint(1, 2);
  if (!cell.isEmpty()) {
    // Do not cover left side of the cell in order to enable grabbing the drag
    // handle
    if (o->isVerticalTimeline()) {
      int dragHandleWidth = o->rect(PredefinedRect::DRAG_HANDLE_CORNER).width();
      setFixedSize(o->cellWidth() - dragHandleWidth, o->cellHeight() + 2);
      move(xy + QPoint(1 + dragHandleWidth, 1));
    } else {
      setFixedSize(o->cellWidth(), o->cellHeight() + 2);
      move(xy + QPoint(1, 1));
    }

    TFrameId fid           = cell.getFrameId();
    std::wstring levelName = cell.m_level->getName();

    QString cellTxt;
    int topPad   = o->isVerticalTimeline() ? 0 : 3;
    int rightPad = o->isVerticalTimeline() ? 0 : 1;
    // show text for the sound text levels
    if (cell.m_level->getType() == TXshLevelType::SND_TXT_XSHLEVEL) {
      TXshSoundTextLevelP textLevel = cell.m_level->getSoundTextLevel();
      cellTxt = textLevel->getFrameText(fid.getNumber() - 1);
    } else {
      bool showLevelName = true;
      if (Preferences::instance()->getLevelNameDisplayType() ==
        Preferences::ShowLevelNameOnColumnHeader) {
        std::string columnName = "";
        if (col >= 0 && xsh->getStageObject(TStageObjectId::ColumnId(col))
          ->hasSpecifiedName())
          columnName =
          xsh->getStageObject(TStageObjectId::ColumnId(col))->getName();
        showLevelName = !checkContainsSingleLevel(xsh->getColumn(col), columnName);
      }

      // convert the last one digit of the frame number to alphabet
      // Ex.  12 -> 1B    21 -> 2A   30 -> 3
      if (Preferences::instance()->isShowFrameNumberWithLettersEnabled()) {
        cellTxt =
            (fid.isEmptyFrame() || fid.isNoFrame())
                ? QString::fromStdWString(levelName)
            : (multiColumnSelected || !showLevelName)
                ? m_viewer->getFrameNumberWithLetters(fid.getNumber())
                : QString::fromStdWString(levelName) + QString(" ") +
                      m_viewer->getFrameNumberWithLetters(fid.getNumber());
      } else {
        QString frameNumber("");
        if (fid.getNumber() > 0) frameNumber = QString::number(fid.getNumber());
        if (!fid.getLetter().isEmpty()) frameNumber += fid.getLetter();

        cellTxt = (frameNumber.isEmpty()) ? QString::fromStdWString(levelName)
                  : (multiColumnSelected || !showLevelName)
                      ? frameNumber
                      : QString::fromStdWString(levelName) + QString(" ") +
                            frameNumber;
      }
      Qt::Alignment alignFlag =
          ((showLevelName) ? Qt::AlignRight | Qt::AlignBottom
                           : Qt::AlignCenter);
      setAlignment(alignFlag);

      if (o->isVerticalTimeline() && fid.getNumber() > 0 && !showLevelName) {
        rightPad = 5;
        if (Preferences::instance()->isShowKeyframesOnXsheetCellAreaEnabled()) {
          TStageObject *pegbar =
              xsh->getStageObject(m_viewer->getObjectId(col));
          int r0, r1;
          if (pegbar && pegbar->getKeyframeRange(r0, r1)) {
            rightPad += o->rect(PredefinedRect::KEY_ICON).width();
          }
        }
      }
    }
    setText(cellTxt);
    setStyleSheet(QString("padding: %1 %2 0 1;").arg(topPad).arg(rightPad));

    selectAll();
  }
  // clear the field if the empty cell is clicked
  else {
    setFixedSize(o->cellWidth(), o->cellHeight() + 2);
    move(xy + QPoint(1, 1));

    setText("");
  }
  show();
  raise();
  setFocus();
}

//-----------------------------------------------------------------------------

void RenameCellField::renameSoundTextColumn(TXshSoundTextColumn *sndTextCol,
                                            const QString &s) {
  if (sndTextCol->isLocked()) return;
  TXsheet *xsheet = m_viewer->getXsheet();

  int r0, c0, r1, c1;
  TCellSelection *cellSelection = dynamic_cast<TCellSelection *>(
      TApp::instance()->getCurrentSelection()->getSelection());
  if (!cellSelection) {
    r0 = m_row;
    r1 = m_row;
  } else
    cellSelection->getSelectedCells(r0, c0, r1, c1);

  TUndoManager::manager()->beginBlock();
  for (int row = r0; row <= r1; row++) {
    QString oldText  = "changeMe";  // text for undo - changed later
    TXshCell cell    = xsheet->getCell(row, m_col);
    TXshCell oldCell = cell;
    // the text index is always one less than the frame number
    int textIndex = cell.getFrameId().getNumber() - 1;
    if (!cell.m_level) {  // cell not part of a level
      oldText       = "";
      int lastFrame = sndTextCol->getMaxFrame();
      TXshSoundTextLevel *sndTextLevel;
      TXshCell lastCell;
      TFrameId newId;
      if (lastFrame < 0) {  // no level on column
        sndTextLevel = new TXshSoundTextLevel();
        sndTextLevel->setType(SND_TXT_XSHLEVEL);
        newId = TFrameId(1);
        cell  = TXshCell(sndTextLevel, newId);
        sndTextCol->setCell(row, cell);
        textIndex = 0;
      } else {
        TXshCell lastCell = xsheet->getCell(lastFrame, m_col);
        TXshSoundTextLevel *sndTextLevel =
            lastCell.m_level->getSoundTextLevel();
        int textSize = sndTextLevel->m_framesText.size();
        textIndex    = textSize;
        newId        = TFrameId(textSize + 1);
        cell         = TXshCell(sndTextLevel, newId);
        sndTextCol->setCell(row, cell);
      }
    }

    TXshCell prevCell             = xsheet->getCell(row - 1, m_col);
    TXshSoundTextLevel *textLevel = cell.m_level->getSoundTextLevel();
    if (oldText == "changeMe")
      oldText = textLevel->getFrameText(cell.getFrameId().getNumber() - 1);
    if (!prevCell.isEmpty()) {
      QString prevCellText =
          textLevel->getFrameText(prevCell.getFrameId().getNumber() - 1);
      // check if the previous cell had the same content as the entered text
      // just extend the frame if so
      // Pressing enter with empty input field will also continue the previous
      // cell text.
      if (prevCellText == s || s.isEmpty()) {
        sndTextCol->setCell(row, prevCell);
        RenameTextCellUndo *undo = new RenameTextCellUndo(
            row, m_col, oldCell, prevCell, oldText, prevCellText, textLevel);
        TUndoManager::manager()->add(undo);
        continue;
      }
      // check if the cell was part of an extended frame, but now has different
      // text
      else if (textLevel->getFrameText(textIndex) == prevCellText &&
               textLevel->getFrameText(textIndex) != s) {
        int textSize   = textLevel->m_framesText.size();
        textIndex      = textSize;
        TFrameId newId = TFrameId(textSize + 1);
        cell           = TXshCell(textLevel, newId);
        sndTextCol->setCell(row, cell);
      }
    }
    RenameTextCellUndo *undo = new RenameTextCellUndo(row, m_col, oldCell, cell,
                                                      oldText, s, textLevel);
    TUndoManager::manager()->add(undo);
    textLevel->setFrameText(textIndex, s);
  }
  TUndoManager::manager()->endBlock();
  TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();
}

//-----------------------------------------------------------------------------

void RenameCellField::renameCell() {
  QString newName = text();
  // std::wstring newName = s.toStdWString();

  setText("");

  std::wstring levelName;
  TFrameId fid;

  TXsheet *xsheet = m_viewer->getXsheet();

  // renaming note level column
  if (xsheet->getColumn(m_col) &&
      xsheet->getColumn(m_col)->getSoundTextColumn()) {
    TXshSoundTextColumn *sndTextCol =
        xsheet->getColumn(m_col)->getSoundTextColumn();
    renameSoundTextColumn(sndTextCol, newName);
    return;
  }

  // convert the last one digit of the frame number to alphabet
  // Ex.  12 -> 1B    21 -> 2A   30 -> 3
  if (Preferences::instance()->isShowFrameNumberWithLettersEnabled())
    parse_with_letter(newName, levelName, fid);
  else {
    parse(newName, levelName, fid);
  }
  bool animationSheetEnabled =
      Preferences::instance()->isAnimationSheetEnabled();

  TCellSelection *cellSelection = dynamic_cast<TCellSelection *>(
      TApp::instance()->getCurrentSelection()->getSelection());
  if (!cellSelection) return;

  ToonzScene *scene = m_viewer->getXsheet()->getScene();
  TFrameId tmplFId  = scene->getProperties()->formatTemplateFIdForInput();

  QList<TXshCell> cells;
  bool hasFrameZero = false;

  if (levelName == L"") {
    int r0, c0, r1, c1;
    cellSelection->getSelectedCells(r0, c0, r1, c1);
    bool changed = false;
    // rename cells for each column in the selection
    for (int c = c0; c <= c1; c++) {
      // if there is no level at the current cell, take the level from the
      // previous frames
      // (when editing not empty column)
      if (xsheet->isColumnEmpty(c)) {
        // find a level with name same as the column (if the preferences option
        // is set to do so)
        if (Preferences::instance()->isLinkColumnNameWithLevelEnabled() &&
            xsheet->getStageObject(TStageObjectId::ColumnId(c))
                ->hasSpecifiedName()) {
          std::string columnName =
              xsheet->getStageObject(TStageObjectId::ColumnId(c))->getName();
          TLevelSet *levelSet = scene->getLevelSet();
          TXshLevel *xl       = levelSet->getLevel(to_wstring(columnName));
          TXshSimpleLevel *sl = (xl) ? xl->getSimpleLevel() : nullptr;
          if (sl &&
              (!sl->isEmpty() || sl->getFirstFid() != TFrameId::NO_FRAME)) {
            sl->formatFId(fid, tmplFId);
            cells.append(TXshCell(xl, fid));
            changed      = true;
            hasFrameZero = (fid.getNumber() == 0 && xl->getSimpleLevel() &&
                            xl->getSimpleLevel()->isFid(fid));
            continue;
          }
        }
        cells.append(TXshCell());
        continue;
      }

      TXshCell cell;
      int tmpRow = m_row;
      while (1) {
        cell = xsheet->getCell(tmpRow, c);
        if (!cell.isEmpty() || tmpRow == 0) break;
        tmpRow--;
      }

      // if the level is not found in the previous frames, then search in the
      // following frames
      if (cell.isEmpty()) {
        tmpRow       = m_row + 1;
        int maxFrame = xsheet->getFrameCount();
        while (1) {
          cell = xsheet->getCell(tmpRow, c);
          if (!cell.isEmpty() || tmpRow >= maxFrame) break;
          tmpRow++;
        }
      }

      TXshLevel *xl       = cell.m_level.getPointer();
      TXshSimpleLevel *sl = (xl) ? xl->getSimpleLevel() : nullptr;
      if (!xl ||
          (sl && !sl->isEmpty() && sl->getFirstFid() == TFrameId::NO_FRAME)) {
        cells.append(TXshCell());
        continue;
      }
      // if the next upper cell is empty, then make this cell empty too
      if (fid == TFrameId::NO_FRAME)
        fid = (m_row - tmpRow <= 1) ? cell.m_frameId : TFrameId(0);

      // modify frameId to be with the same frame format as existing frames
      if (sl) sl->formatFId(fid, tmplFId);

      cells.append(TXshCell(xl, fid));
      changed      = true;
      hasFrameZero = (fid.getNumber() == 0 && xl->getSimpleLevel() &&
                      xl->getSimpleLevel()->isFid(fid));
    }
    if (!changed) return;
  } else {
    TLevelSet *levelSet = scene->getLevelSet();
    TXshLevel *xl       = levelSet->getLevel(levelName);
    if (!xl && fid != TFrameId::NO_FRAME) {
      if (animationSheetEnabled) {
        Preferences *pref   = Preferences::instance();
        int levelType       = pref->getDefLevelType();
        xl                  = scene->createNewLevel(levelType, levelName);
        TXshSimpleLevel *sl = xl->getSimpleLevel();
        if (levelType == TZP_XSHLEVEL || levelType == OVL_XSHLEVEL) {
          // modify frameId to be with the same frame format as existing frames
          sl->formatFId(fid, tmplFId);
          sl->setFrame(fid, sl->createEmptyFrame());
        }
      } else
        xl = scene->createNewLevel(TZI_XSHLEVEL, levelName);
    }
    if (!xl) return;

    // modify frameId to be with the same frame format as existing frames
    if (xl->getSimpleLevel()) xl->getSimpleLevel()->formatFId(fid, tmplFId);

    cells.append(TXshCell(xl, fid));
    hasFrameZero = (fid.getNumber() == 0 && xl->getSimpleLevel() &&
                    xl->getSimpleLevel()->isFid(fid));
  }

  if (fid.getNumber() == 0 && !hasFrameZero) {
    TCellSelection::Range range = cellSelection->getSelectedCells();
    // clear cells without shifting
    cellSelection->deleteCells(false);
    // revert cell selection
    cellSelection->selectCells(range.m_r0, range.m_c0, range.m_r1, range.m_c1);
  } else if (cells.size() == 1)
    cellSelection->renameCells(cells[0]);
  else
    cellSelection->renameMultiCells(cells);
}

//-----------------------------------------------------------------------------

void RenameCellField::onReturnPressed() {
  renameCell();

  // move the cell selection
  TCellSelection *cellSelection = dynamic_cast<TCellSelection *>(
      TApp::instance()->getCurrentSelection()->getSelection());
  if (!cellSelection) return;
  TCellSelection::Range range = cellSelection->getSelectedCells();
  int offset                  = range.m_r1 - range.m_r0 + 1;
  cellSelection->selectCells(range.m_r0 + offset, range.m_c0,
                             range.m_r1 + offset, range.m_c1);
  showInRowCol(m_row + offset, m_col, range.getColCount() > 1);
  m_viewer->updateCells();
  TApp::instance()->getCurrentSelection()->notifySelectionChanged();
}

//-----------------------------------------------------------------------------

void RenameCellField::focusOutEvent(QFocusEvent *e) {
  hide();

  QLineEdit::focusOutEvent(e);
}

//-----------------------------------------------------------------------------
// Override shortcut keys for cell selection commands

bool RenameCellField::eventFilter(QObject *obj, QEvent *e) {
  if (e->type() != QEvent::ShortcutOverride)
    return QLineEdit::eventFilter(obj, e);  // return false;

  TCellSelection *cellSelection = dynamic_cast<TCellSelection *>(
      TApp::instance()->getCurrentSelection()->getSelection());
  if (!cellSelection) return QLineEdit::eventFilter(obj, e);

  QKeyEvent *ke = (QKeyEvent *)e;
  std::string keyStr =
      QKeySequence(ke->key() + ke->modifiers()).toString().toStdString();
  QAction *action = CommandManager::instance()->getActionFromShortcut(keyStr);
  if (!action) return QLineEdit::eventFilter(obj, e);

  std::string actionId = CommandManager::instance()->getIdFromAction(action);

  // These are usually standard ctrl/command strokes for text editing.
  // Default to standard behavior and don't execute OT's action while renaming
  // cell if users prefer to do so.
  // Or, always invoke OT's commands when renaming cell even the standard
  // command strokes for text editing.
  // The latter option is demanded by Japanese animation industry in order to
  // gain efficiency for inputting xsheet.
  if (!Preferences::instance()->isShortcutCommandsWhileRenamingCellEnabled() &&
      (actionId == "MI_Undo" || actionId == "MI_Redo" ||
       actionId == "MI_Clear" || actionId == "MI_Copy" ||
       actionId == "MI_Paste" || actionId == "MI_Cut"))
    return QLineEdit::eventFilter(obj, e);

  return TCellSelection::isEnabledCommand(actionId);
}

//-----------------------------------------------------------------------------

void RenameCellField::keyPressEvent(QKeyEvent *event) {
  if (event->key() == Qt::Key_Escape) {
    clearFocus();
    return;
  }

  // move the cell selection
  TCellSelection *cellSelection = dynamic_cast<TCellSelection *>(
      TApp::instance()->getCurrentSelection()->getSelection());
  if (!cellSelection) {
    QLineEdit::keyPressEvent(event);
    return;
  }

  int r0, c0, r1, c1;
  CellPosition stride(1, 1);  // stride in row and column axes
  cellSelection->getSelectedCells(r0, c0, r1, c1);
  stride.setFrame(cellSelection->getSelectedCells().getRowCount());

  CellPosition offset;
  switch (int key = event->key()) {
  case Qt::Key_Up:
  case Qt::Key_Down:
    offset = m_viewer->orientation()->arrowShift(key);
    break;
  case Qt::Key_Left:
  case Qt::Key_Right:
    // ctrl+left/right arrow for moving cursor to the end in the field
    if (isCtrlPressed &&
        !Preferences::instance()->isUseArrowKeyToShiftCellSelectionEnabled()) {
      QLineEdit::keyPressEvent(event);
      return;
    }
    offset = m_viewer->orientation()->arrowShift(key);
    break;
  case Qt::Key_Shift:
    // prevent the field to lose focus when typing shift key
    event->accept();
    return;
    break;
  default:
    QLineEdit::keyPressEvent(event);
    return;
    break;
  }

  if (isCtrlPressed &&
      Preferences::instance()->isUseArrowKeyToShiftCellSelectionEnabled()) {
    if (r0 == r1 && offset.frame() == -1) return;
    if (c0 == c1 && offset.layer() == -1) return;
    cellSelection->selectCells(r0, c0, r1 + offset.frame(),
                               c1 + offset.layer());
  } else {
    CellPosition offset_(offset * stride);
    int movedR0 = std::max(0, r0 + offset_.frame());
    int firstCol =
        Preferences::instance()->isXsheetCameraColumnVisible() ? -1 : 0;
    int movedC0   = std::max(firstCol, c0 + offset_.layer());
    int diffFrame = movedR0 - r0;
    int diffLayer = movedC0 - c0;
    // It needs to be discussed - I made not to rename cell with arrow key.
    // 19/Jan/2017 shun-iwasawa
    // renameCell();
    cellSelection->selectCells(r0 + diffFrame, c0 + diffLayer, r1 + diffFrame,
                               c1 + diffLayer);
    int newRow = std::max(0, m_row + offset_.frame());
    int newCol = std::max(firstCol, m_col + offset_.layer());
    showInRowCol(newRow, newCol, c1 - c0 > 0);
  }
  m_viewer->updateCells();
  TApp::instance()->getCurrentSelection()->notifySelectionChanged();
}

//-----------------------------------------------------------------------------

void RenameCellField::showEvent(QShowEvent *) {
  bool ret = connect(TApp::instance()->getCurrentXsheet(),
                     SIGNAL(xsheetChanged()), this, SLOT(onXsheetChanged()));
  assert(ret);
}

//-----------------------------------------------------------------------------

void RenameCellField::hideEvent(QHideEvent *) {
  disconnect(TApp::instance()->getCurrentXsheet(), SIGNAL(xsheetChanged()),
             this, SLOT(onXsheetChanged()));
}

//-----------------------------------------------------------------------------

void RenameCellField::onXsheetChanged() {
  TCellSelection *cellSelection = dynamic_cast<TCellSelection *>(
      TApp::instance()->getCurrentSelection()->getSelection());
  if (!cellSelection) {
    hide();
    return;
  }
  TCellSelection::Range range = cellSelection->getSelectedCells();
  showInRowCol(m_row, m_col, range.getColCount() > 1);
}

//=============================================================================
// CellArea
//-----------------------------------------------------------------------------

CellArea::CellArea(XsheetViewer *parent, Qt::WindowFlags flags)
    : CustomContextMenuWidget("XsheetCellArea", "XsheetGUI::CellArea", parent,
                              flags)
    , m_viewer(parent)
    , m_levelExtenderRect()
    , m_soundLevelModifyRects()
    , m_isPanning(false)
    , m_isMousePressed(false)
    , m_pos(-1, -1)
    , m_tooltip(tr(""))
    , m_renameCell(new RenameCellField(this, m_viewer)) {
  setAcceptDrops(true);
  setMouseTracking(true);
  m_renameCell->hide();
  setFocusPolicy(Qt::NoFocus);
  setObjectName("XsheetCellArea");
}

//-----------------------------------------------------------------------------

CellArea::~CellArea() {}

//-----------------------------------------------------------------------------

DragTool *CellArea::getDragTool() const { return m_viewer->getDragTool(); }
void CellArea::setDragTool(DragTool *dragTool) {
  m_viewer->setDragTool(dragTool);
}

//-----------------------------------------------------------------------------

void CellArea::drawFrameSeparator(QPainter &p, int row, int col,
                                  bool emptyFrame, bool heldFrame) {
  const Orientation *o = m_viewer->orientation();
  int layerAxis        = m_viewer->columnToLayerAxis(col);

  NumberRange layerAxisRange(layerAxis + 1,
                             m_viewer->columnToLayerAxis(col + 1));
  if (!o->isVerticalTimeline()) {
    int adjY       = o->cellHeight() - 1;
    layerAxisRange = NumberRange(layerAxis + 1, layerAxis + adjY);
  }

  // mark interval
  int distance, offset, secDistance;
  TApp::instance()->getCurrentScene()->getScene()->getProperties()->getMarkers(
      distance, offset, secDistance);

  bool isAfterMarkers =
      distance > 0 && ((row - offset) % distance) == 0 && row != 0;
  bool isAfterSecMarkers =
      secDistance > 0 && ((row - offset) % secDistance) == 0 && row != 0;

  TCellSelection *cellSelection = m_viewer->getCellSelection();
  bool isSelected               = cellSelection->isCellSelected(row, col);

  QColor color     = (isAfterMarkers || isAfterSecMarkers)
                         ? (isSelected) ? m_viewer->getSelectedMarkerLineColor()
                           : (isAfterSecMarkers) ? m_viewer->getSecMarkerLineColor()
                                                 : m_viewer->getMarkerLineColor()
                         : m_viewer->getLightLineColor();
  double lineWidth = (isAfterSecMarkers)                   ? 3.
                     : (secDistance > 0 && isAfterMarkers) ? 2.
                                                           : 1.;

  int frameAxis = m_viewer->rowToFrameAxis(row);
  int handleSize =
      (emptyFrame) ? 0
      : (o->isVerticalTimeline())
          ? (isAfterMarkers || isAfterSecMarkers)
                ? 0
                : o->rect(PredefinedRect::DRAG_HANDLE_CORNER).width()
          : o->rect(PredefinedRect::DRAG_HANDLE_CORNER).height();

  QLine horizontalLine = m_viewer->orientation()->horizontalLine(
      frameAxis, layerAxisRange.adjusted(handleSize - 1, 1));
  if (heldFrame) {
    int x = horizontalLine.x1();
    int y = horizontalLine.y2() - 2;
    horizontalLine.setP1(QPoint(x, y));
    color.setAlpha(120);
  } else if (!o->isVerticalTimeline() && !isAfterMarkers && emptyFrame)
    color.setAlpha(m_viewer->getLightLineColor().alpha());

  p.setPen(QPen(color, lineWidth, Qt::SolidLine, Qt::FlatCap));
  p.drawLine(horizontalLine);
}

//-----------------------------------------------------------------------------

void CellArea::drawCells(QPainter &p, const QRect toBeUpdated) {
  TXsheet *xsh         = m_viewer->getXsheet();
  const Orientation *o = m_viewer->orientation();
  ColumnFan *columnFan = xsh->getColumnFan(o);

  // selected cells range
  TCellSelection *cellSelection = m_viewer->getCellSelection();
  int rS0, cS0, rS1, cS1;
  if (!cellSelection->isEmpty())
    cellSelection->getSelectedCells(rS0, cS0, rS1, cS1);
  // visible cells range
  CellRange visible = m_viewer->xyRectToRange(toBeUpdated);
  int r0, r1, c0, c1;  // range of visible rows and columns
  r0 = visible.from().frame();
  r1 = visible.to().frame();
  c0 = visible.from().layer();
  c1 = visible.to().layer();
  if (!m_viewer->orientation()->isVerticalTimeline()) {
    int colCount = std::max(1, xsh->getColumnCount());
    c1           = std::min(c1, colCount - 1);
  }

  drawNonEmptyBackground(p);

  drawSelectionBackground(p);

  int currentRow = m_viewer->getCurrentRow();
  int col, row;

  int drawLeft  = std::max(1, toBeUpdated.left());
  int drawRight = std::min(width(), toBeUpdated.right());

  int drawTop    = std::max(1, toBeUpdated.top());
  int drawBottom = std::min(height() - 2, toBeUpdated.bottom());
  QRect drawingArea(QPoint(drawLeft, drawTop), QPoint(drawRight, drawBottom));
  NumberRange frameSide = m_viewer->orientation()->frameSide(drawingArea);

  // for each layer
  m_soundLevelModifyRects.clear();
  for (col = c0; col <= c1; col++) {
    // x in vertical timeline / y in horizontal
    int layerAxis = m_viewer->columnToLayerAxis(col);

    if (!columnFan->isActive(col)) {  // folded column
      drawFoldedColumns(p, layerAxis, frameSide);

      if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
          Preferences::instance()->isCurrentTimelineIndicatorEnabled()) {
        row       = m_viewer->getCurrentRow();
        QPoint xy = m_viewer->positionToXY(CellPosition(row, col));
        int x     = xy.x();
        int y     = xy.y();
        if (row == 0) {
          if (m_viewer->orientation()->isVerticalTimeline())
            xy.setY(xy.y() + 1);
          else
            xy.setX(xy.x() + 1);
        }
        drawCurrentTimeIndicator(p, xy, true);
      }
      continue;
    }

    TXshColumn *column     = xsh->getColumn(col);
    bool isColumn          = (column) ? true : false;
    bool isSoundColumn     = false;
    bool isPaletteColumn   = false;
    bool isSoundTextColumn = false;
    bool showLevelName     = true;
    if (isColumn) {
      isSoundColumn     = column->getSoundColumn() != 0;
      isPaletteColumn   = column->getPaletteColumn() != 0;
      isSoundTextColumn = column->getSoundTextColumn() != 0;
      if (Preferences::instance()->getLevelNameDisplayType() ==
          Preferences::ShowLevelNameOnColumnHeader) {
        std::string columnName = "";
        if (col >= 0 && xsh->getStageObject(TStageObjectId::ColumnId(col))
                            ->hasSpecifiedName())
          columnName =
              xsh->getStageObject(TStageObjectId::ColumnId(col))->getName();
        showLevelName =
            (isSoundColumn || isPaletteColumn || isSoundTextColumn ||
             !checkContainsSingleLevel(column, columnName));
      }
    }
    // check if the column is reference
    bool isReference = true;
    if (column) {  // Verifico se la colonna e' una mask
      if (column->isControl() || column->isRendered() ||
          column->getMeshColumn())
        isReference = false;
    }

    if (isSoundTextColumn)
      drawSoundTextColumn(p, r0, r1, col);
    else {
      // for each frame
      for (row = r0; row <= r1; row++) {
        if (col >= 0 && !isColumn) {
          drawFrameSeparator(p, row, col, true);
          if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
              row == m_viewer->getCurrentRow() &&
              Preferences::instance()->isCurrentTimelineIndicatorEnabled()) {
            QPoint xy = m_viewer->positionToXY(CellPosition(row, col));
            int x     = xy.x();
            int y     = xy.y();
            if (row == 0) {
              if (m_viewer->orientation()->isVerticalTimeline())
                xy.setY(xy.y() + 1);
              else
                xy.setX(xy.x() + 1);
            }
            drawCurrentTimeIndicator(p, xy, col);
          }
          continue;
        }
        // Cells appearance depending on the type of column
        if (isSoundColumn)
          drawSoundCell(p, row, col, isReference);
        else if (isPaletteColumn)
          drawPaletteCell(p, row, col, isReference);
        // else if (isSoundTextColumn) // I left these lines just in case
        //  drawSoundTextCell(p, row, col);
        else
          drawLevelCell(p, row, col, isReference, showLevelName);
      }
    }

    // draw vertical line
    if (layerAxis > 0) {
      p.setPen(m_viewer->getVerticalLineColor());
      QLine verticalLine =
          m_viewer->orientation()->verticalLine(layerAxis, frameSide);
      p.drawLine(verticalLine);
    }
  }

  drawExtenderHandles(p);
}

// slightly bright background for non-empty rectangular area
void CellArea::drawNonEmptyBackground(QPainter &p) const {
  TXsheet *xsh = m_viewer->getXsheet();

  int totalFrames = xsh->getFrameCount();
  if (!totalFrames) return;

  int lastNonEmptyCol;
  for (lastNonEmptyCol = xsh->getColumnCount() - 1; lastNonEmptyCol >= 0;
       lastNonEmptyCol--) {
    TXshColumn *currentColumn = xsh->getColumn(lastNonEmptyCol);
    if (!currentColumn) continue;
    if (!currentColumn->isEmpty()) break;
  }
  QPoint xyTop, xyBottom;
  const Orientation *o = m_viewer->orientation();
  if (o->isVerticalTimeline()) {
    xyTop = QPoint(1, 0);
    xyBottom =
        m_viewer->positionToXY(CellPosition(totalFrames, lastNonEmptyCol + 1));
  } else {
    xyTop          = m_viewer->positionToXY(CellPosition(0, lastNonEmptyCol));
    xyBottom       = m_viewer->positionToXY(CellPosition(totalFrames, -1));
    ColumnFan *fan = xsh->getColumnFan(o);
    xyBottom.setY(xyBottom.y() +
                  ((fan ? fan->isActive(0) : true) ? o->cellHeight() : 9));
  }
  p.fillRect(xyTop.x(), xyTop.y(), xyBottom.x(), xyBottom.y(),
             QBrush(m_viewer->getNotEmptyColumnColor()));
}

void CellArea::drawFoldedColumns(QPainter &p, int layerAxis,
                                 const NumberRange &frameAxis) const {
  // 3 white bars
  for (int i = 0; i < 3; i++) {
    QRect whiteRect =
        m_viewer->orientation()->foldedRectangle(layerAxis, frameAxis, i);
    p.fillRect(whiteRect, QBrush(m_viewer->getFoldedColumnBGColor()));
  }

  // 3 dark lines
  p.setPen(m_viewer->getFoldedColumnLineColor());
  for (int i = 0; i < 3; i++) {
    QLine darkLine =
        m_viewer->orientation()->foldedRectangleLine(layerAxis, frameAxis, i);
    p.drawLine(darkLine);
  }
}

void CellArea::drawSelectionBackground(QPainter &p) const {
  // selected cells range
  TCellSelection *cellSelection = m_viewer->getCellSelection();
  if (cellSelection->isEmpty()) return;

  int selRow0, selCol0, selRow1, selCol1;
  cellSelection->getSelectedCells(selRow0, selCol0, selRow1, selCol1);
  QRect selectionRect;
  const Orientation *o = m_viewer->orientation();
  if (o->isVerticalTimeline())
    selectionRect = m_viewer->rangeToXYRect(
        CellRange(CellPosition(selRow0, selCol0),
                  CellPosition(selRow1 + 1, selCol1 + 1)));
  else {
    int newSelCol0 = std::max(selCol0, selCol1);
    int newSelCol1 = std::min(selCol0, selCol1);
    selectionRect  = m_viewer->rangeToXYRect(
        CellRange(CellPosition(selRow0, newSelCol0),
                   CellPosition(selRow1 + 1, newSelCol1 - 1)));
  }

  p.fillRect(selectionRect, QBrush(m_viewer->getSelectedEmptyCellColor()));
}

void CellArea::drawExtenderHandles(QPainter &p) {
  const Orientation *o = m_viewer->orientation();

  m_levelExtenderRect      = QRect();
  m_upperLevelExtenderRect = QRect();

  // selected cells range
  TCellSelection *cellSelection = m_viewer->getCellSelection();
  if (cellSelection->isEmpty() || m_viewer->areSoundCellsSelected()) return;

  // if the drag move is disabled, the extender handles won't appear
  if (Preferences::instance()->getDragCellsBehaviour() == 2) return;

  int selRow0, selCol0, selRow1, selCol1;
  cellSelection->getSelectedCells(selRow0, selCol0, selRow1, selCol1);

  // do nothing if only the camera column is selected
  if (selCol1 < 0) return;

  QRect selected;
  selected = m_viewer->rangeToXYRect(CellRange(
      CellPosition(selRow0, selCol0), CellPosition(selRow1 + 1, selCol1 + 1)));
  if (!o->isVerticalTimeline()) {
    TXsheet *xsh         = m_viewer->getXsheet();
    ColumnFan *columnFan = xsh->getColumnFan(o);
    int topAdj           = columnFan->isActive(selCol0) ? o->cellHeight() : 9;
    int bottomAdj        = columnFan->isActive(selCol1) ? o->cellHeight() : 9;
    selected.adjust(0, topAdj, 0, bottomAdj);
  }

  int x0, y0, x1, y1;
  x0 = selected.left();
  x1 = selected.right();
  y0 = selected.top();
  y1 = selected.bottom();

  // smart tab
  QPoint smartTabPosOffset =
      Preferences::instance()->isShowKeyframesOnXsheetCellAreaEnabled()
          ? QPoint(0, 0)
          : o->point(PredefinedPoint::KEY_HIDDEN);

  int distance, offset, secDistance;
  TApp::instance()->getCurrentScene()->getScene()->getProperties()->getMarkers(
      distance, offset, secDistance);

  QPoint xyRadius = o->point(PredefinedPoint::EXTENDER_XY_RADIUS);

  // bottom / right extender handle
  m_levelExtenderRect =
      o->rect(PredefinedRect::END_EXTENDER)
          .translated(selected.bottomRight() + smartTabPosOffset);
  p.setPen(Qt::black);
  p.setBrush(SmartTabColor);
  p.drawRoundRect(m_levelExtenderRect, xyRadius.x(), xyRadius.y());
  QColor color = (distance > 0 && ((selRow1 + 1 - offset) % distance) != 0)
                     ? m_viewer->getLightLineColor()
                     : m_viewer->getMarkerLineColor();
  p.setPen(color);
  QLine extenderLine = o->line(PredefinedLine::EXTENDER_LINE);
  extenderLine.setP1(extenderLine.p1() + smartTabPosOffset);
  p.drawLine(extenderLine.translated(selected.bottomRight()));

  // up / left extender handle
  if (isCtrlPressed && selRow0 > 0 && !m_viewer->areCellsSelectedEmpty()) {
    QPoint properPoint       = o->topRightCorner(selected);
    m_upperLevelExtenderRect = o->rect(PredefinedRect::BEGIN_EXTENDER)
                                   .translated(properPoint + smartTabPosOffset);
    p.setPen(Qt::black);
    p.setBrush(SmartTabColor);
    p.drawRoundRect(m_upperLevelExtenderRect, xyRadius.x(), xyRadius.y());
    QColor color = (distance > 0 && ((selRow0 - offset) % distance) != 0)
                       ? m_viewer->getLightLineColor()
                       : m_viewer->getMarkerLineColor();
    p.setPen(color);
    p.drawLine(extenderLine.translated(properPoint));
  }

  p.setBrush(Qt::NoBrush);
}

//-----------------------------------------------------------------------------

void CellArea::drawSoundCell(QPainter &p, int row, int col, bool isReference) {
  const Orientation *o = m_viewer->orientation();
  TXshSoundColumn *soundColumn =
      m_viewer->getXsheet()->getColumn(col)->getSoundColumn();
  QPoint xy = m_viewer->positionToXY(CellPosition(row, col));
  int x     = xy.x();
  int y     = xy.y();
  if (row == 0) {
    if (o->isVerticalTimeline())
      xy.setY(xy.y() + 1);
    else
      xy.setX(xy.x() + 1);
  }

  TXshCell nextCell;
  nextCell         = soundColumn->getSoundCell(row + 1);  // cell in next frame
  bool isNextEmpty = nextCell.getFrameId().getNumber() < 0;

  QPoint frameAdj = m_viewer->getFrameZoomAdjustment();
  int frameZoomF  = m_viewer->getFrameZoomFactor();
  QRect cellRect  = o->rect(PredefinedRect::CELL).translated(QPoint(x, y));
  cellRect.adjust(0, 0, -frameAdj.x(), -frameAdj.y());
  QRect rect = cellRect.adjusted(
      1, 1,
      (!m_viewer->orientation()->isVerticalTimeline() && !isNextEmpty ? 2 : 0),
      0);

  int markId = soundColumn->getCellMark(row);
  QColor markColor;
  if (markId >= 0) {
    TPixel32 col = TApp::instance()
                       ->getCurrentScene()
                       ->getScene()
                       ->getProperties()
                       ->getCellMark(markId)
                       .color;
    markColor = QColor(col.r, col.g, col.b, 196);  // semi transparent
  }
  QRect markRect =
      o->rect(PredefinedRect::CELL_MARK_AREA)
          .adjusted(0, -std::round(double(frameAdj.y()) * 0.1), -frameAdj.y(),
                    -std::round(double(frameAdj.y()) * 0.9))
          .translated(xy);

  int maxNumFrame = soundColumn->getMaxFrame() + 1;
  int startFrame  = soundColumn->getFirstRow();
  TXshCell cell   = soundColumn->getSoundCell(row);
  if (soundColumn->isCellEmpty(row) || cell.isEmpty() || row > maxNumFrame ||
      row < startFrame) {
    drawFrameSeparator(p, row, col, true);
    if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
        row == m_viewer->getCurrentRow() &&
        Preferences::instance()->isCurrentTimelineIndicatorEnabled())
      drawCurrentTimeIndicator(p, xy, col);
    return;
  }

  TXshSoundLevelP soundLevel = cell.getSoundLevel();

  int r0, r1;
  if (!soundColumn->getLevelRange(row, r0, r1)) {
    if (o->isVerticalTimeline() || !row) drawFrameSeparator(p, row, col, false);

    // only draw mark
    if (markId >= 0) {
      p.setBrush(markColor);
      p.setPen(Qt::NoPen);
      p.drawEllipse(markRect);
    }
    return;
  }
  bool isFirstRow = (row == r0);
  bool isLastRow  = (row == r1);

  TCellSelection *cellSelection     = m_viewer->getCellSelection();
  TColumnSelection *columnSelection = m_viewer->getColumnSelection();
  bool isSelected                   = cellSelection->isCellSelected(row, col) ||
                    columnSelection->isColumnSelected(col);

  // get cell colors
  QColor cellColor, sideColor;
  int levelType;
  if (isReference) {
    cellColor = (isSelected) ? m_viewer->getSelectedReferenceColumnColor()
                             : m_viewer->getReferenceColumnColor();
    sideColor = m_viewer->getReferenceColumnBorderColor();
  } else
    m_viewer->getCellTypeAndColors(levelType, cellColor, sideColor, cell,
                                   isSelected);

  // cells background
  if (o->isVerticalTimeline())
    // Paint the cell edge-to-edge, we use LightLineColor with low opacity to
    // pick up the hue of the cell color to make the separator line more
    // pleasing to the eye.
    p.fillRect(rect.adjusted(0, 0, 0, 1), QBrush(cellColor));
  else
    p.fillRect(rect, QBrush(cellColor));

  if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
      row == m_viewer->getCurrentRow() &&
      Preferences::instance()->isCurrentTimelineIndicatorEnabled())
    drawCurrentTimeIndicator(p, xy, col);

  drawDragHandle(p, xy, sideColor);
  drawEndOfDragHandle(p, isLastRow, xy, cellColor);
  drawLockedDottedLine(p, soundColumn->isLocked(), xy, cellColor);

  if (o->isVerticalTimeline() || !row) drawFrameSeparator(p, row, col, false);

  QRect trackRect = o->rect(PredefinedRect::SOUND_TRACK)
                        .adjusted(0, 0, -frameAdj.x(), -frameAdj.y())
                        .translated(xy);
  QRect previewRect = o->rect(PredefinedRect::PREVIEW_TRACK)
                          .adjusted(0, 0, -frameAdj.x(), -frameAdj.y())
                          .translated(xy);
  NumberRange trackBounds   = o->layerSide(trackRect);
  NumberRange previewBounds = o->layerSide(previewRect);
  NumberRange trackAndPreview(trackBounds.from(), previewBounds.to());

  NumberRange timeBounds = o->frameSide(trackRect);
  int offset =
      row - cell.getFrameId().getNumber();  // rows since start of the clip
  int begin      = timeBounds.from();       // time axis
  int end        = timeBounds.to();
  int soundPixel = o->rowToFrameAxis(row) -
                   o->rowToFrameAxis(offset);  // pixels since start of clip

  int trackWidth = trackBounds.length();
  DoublePair minmax;
  soundLevel->getValueAtPixel(o, soundPixel, minmax);

  double pmin = minmax.first;
  double pmax = minmax.second;

  int center = trackBounds.middle();

  bool scrub = m_viewer->isScrubHighlighted(row, col);

  int i;
  int z = 100 / frameZoomF;
  for (i = begin; i <= end; i++) {
    soundLevel->getValueAtPixel(o, soundPixel, minmax);
    soundPixel += z;  // ++;
    int min, max;
    pmin = minmax.first;
    pmax = minmax.second;

    min = tcrop((int)pmin, -trackWidth / 2, 0) + center;
    max = tcrop((int)pmax, 0, trackWidth / 2 - 1) + center;

    if (scrub && i % 2) {
      p.setPen(m_viewer->getSoundColumnHlColor());
      QLine stroke = o->horizontalLine(i, previewBounds.adjusted(-1, -1));
      p.drawLine(stroke);
    } else if (i != begin || !isFirstRow) {
      // preview tool on the right side
      if (i % 2)
        p.setPen(cellColor);
      else
        p.setPen(m_viewer->getSoundColumnTrackColor());
      QLine stroke = o->horizontalLine(i, previewBounds.adjusted(-1, -1));
      p.drawLine(stroke);
    }
    if (!o->isVerticalTimeline() || i != begin) {
      // "audio track" in the middle of the column
      p.setPen(m_viewer->getSoundColumnTrackColor());
      QLine midLine = o->horizontalLine(i, NumberRange(min, max));
      p.drawLine(midLine);
    }
  }

  // yellow clipped border
  p.setPen(SoundColumnExtenderColor);
  int r0WithoutOff, r1WithoutOff;
  bool ret =
      soundColumn->getLevelRangeWithoutOffset(row, r0WithoutOff, r1WithoutOff);
  assert(ret);

  if (isFirstRow) {
    QRect modifierRect = m_viewer->orientation()
                             ->rect(PredefinedRect::BEGIN_SOUND_EDIT)
                             .translated(xy);
    if (r0 != r0WithoutOff) p.fillRect(modifierRect, SoundColumnExtenderColor);
    m_soundLevelModifyRects.append(modifierRect);  // list of clipping rects
  }
  if (isLastRow) {
    QRect modifierRect = m_viewer->orientation()
                             ->rect(PredefinedRect::END_SOUND_EDIT)
                             .translated(-frameAdj)
                             .translated(xy);
    if (r1 != r1WithoutOff) p.fillRect(modifierRect, SoundColumnExtenderColor);
    m_soundLevelModifyRects.append(modifierRect);
  }

  // cell mark
  if (markId >= 0) {
    p.setBrush(markColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(markRect);
  }
}

//-----------------------------------------------------------------------------

// paint side bar
void CellArea::drawDragHandle(QPainter &p, const QPoint &xy,
                              const QColor &sideColor) const {
  QPoint frameAdj      = m_viewer->getFrameZoomAdjustment();
  QRect dragHandleRect = m_viewer->orientation()
                             ->rect(PredefinedRect::DRAG_HANDLE_CORNER)
                             .adjusted(0, 0, -frameAdj.x(), -frameAdj.y())
                             .translated(xy);
  p.fillRect(dragHandleRect, QBrush(sideColor));
}

// cut off triangle at the end of drag handle
void CellArea::drawEndOfDragHandle(QPainter &p, bool isEnd, const QPoint &xy,
                                   const QColor &cellColor) const {
  if (!isEnd) return;
  QPoint frameAdj     = m_viewer->getFrameZoomAdjustment();
  QPainterPath corner = m_viewer->orientation()
                            ->path(PredefinedPath::DRAG_HANDLE_CORNER)
                            .translated(xy - frameAdj);
  p.fillPath(corner, QBrush(cellColor));
}

// draw dot line if the column is locked
void CellArea::drawLockedDottedLine(QPainter &p, bool isLocked,
                                    const QPoint &xy,
                                    const QColor &cellColor) const {
  if (!isLocked) return;
  p.setPen(QPen(cellColor, 2, Qt::DotLine));
  QPoint frameAdj = m_viewer->getFrameZoomAdjustment();
  QLine dottedLine =
      m_viewer->orientation()->line(PredefinedLine::LOCKED).translated(xy);
  dottedLine.setP2(QPoint(dottedLine.x2(), dottedLine.y2()) - frameAdj);
  p.drawLine(dottedLine);
}

void CellArea::drawCurrentTimeIndicator(QPainter &p, const QPoint &xy, int col,
                                        bool isFolded) {
  QPoint frameAdj       = m_viewer->getFrameZoomAdjustment();
  QColor indicatorColor = m_viewer->getCurrentTimeIndicatorColor();
  if (!m_viewer->orientation()->isVerticalTimeline()) {
    QLine line = m_viewer->orientation()
                     ->line(PredefinedLine::CURRENT_TIME_INDICATOR)
                     .translated(xy)
                     .translated(-frameAdj / 2);
    // Adjust for 1st row
    if (xy.x() <= 1) line.translate(-1, 0);
    if (isFolded)
      line.setP2(line.p1() +
                 QPoint(0, m_viewer->orientation()->foldedCellSize() - 1));
    p.setPen(indicatorColor);
    p.drawLine(line);
  } else {
    QRect cellRect = m_viewer->orientation()
                         ->rect((col < 0) ? PredefinedRect::CAMERA_CELL
                                          : PredefinedRect::CELL)
                         .translated(xy)
                         .adjusted(1, 1, -frameAdj.x(), -frameAdj.y());
    // Adjust for 1st row
    if (xy.y() <= 1) cellRect.adjust(0, -1, 0, -1);
    if (isFolded)
      cellRect.setRight(cellRect.left() +
                        m_viewer->orientation()->foldedCellSize() - 1);

    p.setPen(
        QPen(indicatorColor, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
    p.drawLine(cellRect.topLeft(), cellRect.topRight());
    p.drawLine(cellRect.bottomLeft(), cellRect.bottomRight());
  }
}

void CellArea::drawFrameMarker(QPainter &p, const QPoint &xy, QColor color,
                               bool isKeyFrame, bool isCamera) {
  QColor outlineColor = Qt::black;
  QPoint frameAdj     = m_viewer->getFrameZoomAdjustment();
  QRect dotRect       = (isCamera)
                            ? m_viewer->orientation()
                            ->rect(PredefinedRect::CAMERA_FRAME_MARKER_AREA)
                            .translated(xy)
                            .translated(-frameAdj / 2)
                            : m_viewer->orientation()
                            ->rect(PredefinedRect::FRAME_MARKER_AREA)
                            .translated(xy)
                            .translated(-frameAdj / 2);

  if (isKeyFrame)
    m_viewer->drawPredefinedPath(p, PredefinedPath::FRAME_MARKER_DIAMOND,
                                 dotRect.adjusted(1, 1, 1, 1).center(), color,
                                 outlineColor);
  else {
    // move to column center
    if (m_viewer->orientation()->isVerticalTimeline()) {
      PredefinedLine which =
          Preferences::instance()->isLevelNameOnEachMarkerEnabled()
              ? PredefinedLine::CONTINUE_LEVEL_WITH_NAME
              : PredefinedLine::CONTINUE_LEVEL;

      QLine continueLine = m_viewer->orientation()->line(which).translated(xy);
      dotRect.moveCenter(QPoint(continueLine.x1() - 1, dotRect.center().y()));
    }
    p.setPen(outlineColor);
    p.setBrush(color);
    p.drawEllipse(dotRect);
    p.setBrush(Qt::NoBrush);
  }
}

//-----------------------------------------------------------------------------

void CellArea::drawFocusCellBorder(QPainter &p) {
  QColor color = m_viewer->getCellFocusColor();
  if (color.alpha() == 0) return;
  QPoint frameAdj = m_viewer->getFrameZoomAdjustment();
  int row         = m_viewer->getCurrentRow();
  int col         = m_viewer->getCurrentColumn();
  QPoint xy       = m_viewer->positionToXY(CellPosition(row, col));
  QRect rect =
      m_viewer->orientation()
          ->rect((col < 0) ? PredefinedRect::CAMERA_CELL : PredefinedRect::CELL)
          .translated(xy)
          .adjusted(1, 1, -1 - frameAdj.x(), -frameAdj.y());
  p.setPen(QPen(color, 2, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
  p.setBrush(Qt::NoBrush);
  p.drawRect(rect);
}

//-----------------------------------------------------------------------------

void CellArea::drawLevelCell(QPainter &p, int row, int col, bool isReference,
                             bool showLevelName) {
  const Orientation *o = m_viewer->orientation();

  TXsheet *xsh  = m_viewer->getXsheet();
  TXshCell cell = xsh->getCell(row, col);
  TXshCell prevCell;

  TXshCellColumn *cellColumn = xsh->getColumn(col)->getCellColumn();
  int markId                 = (cellColumn) ? cellColumn->getCellMark(row) : -1;
  QColor markColor;
  if (markId >= 0) {
    TPixel32 col = TApp::instance()
                       ->getCurrentScene()
                       ->getScene()
                       ->getProperties()
                       ->getCellMark(markId)
                       .color;
    markColor = QColor(col.r, col.g, col.b, 196);  // semi transparent
  }

  TCellSelection *cellSelection     = m_viewer->getCellSelection();
  TColumnSelection *columnSelection = m_viewer->getColumnSelection();
  bool isSelected                   = cellSelection->isCellSelected(row, col) ||
                    columnSelection->isColumnSelected(col);
  bool isSimpleView = m_viewer->getFrameZoomFactor() <=
                      o->dimension(PredefinedDimension::SCALE_THRESHOLD);

  if (row > 0) prevCell = xsh->getCell(row - 1, col);  // cell in previous frame

  bool sameLevel = prevCell.m_level.getPointer() == cell.m_level.getPointer();

  QPoint xy = m_viewer->positionToXY(CellPosition(row, col));
  int x     = xy.x();
  int y     = xy.y();
  if (row == 0) {
    if (o->isVerticalTimeline())
      xy.setY(xy.y() + 1);
    else
      xy.setX(xy.x() + 1);
  }

  TXshCell nextCell;
  nextCell = xsh->getCell(row + 1, col);  // cell in next frame

  QPoint frameAdj = m_viewer->getFrameZoomAdjustment();
  QRect cellRect =
      o->rect((col < 0) ? PredefinedRect::CAMERA_CELL : PredefinedRect::CELL)
          .translated(QPoint(x, y));
  cellRect.adjust(0, 0, -frameAdj.x(), -frameAdj.y());
  QRect rect = cellRect.adjusted(
      1, 1, (!o->isVerticalTimeline() && !nextCell.isEmpty() ? 2 : 0), 0);

  QRect markRect =
      o->rect(PredefinedRect::CELL_MARK_AREA)
          .adjusted(0, -std::round(double(frameAdj.y()) * 0.1), -frameAdj.y(),
                    -std::round(double(frameAdj.y()) * 0.9))
          .translated(xy);
  if (showLevelName && (!isSimpleView || !o->isVerticalTimeline()))
    markRect.moveCenter(cellRect.center());
  if (markRect.right() > rect.right()) markRect.setRight(rect.right());

  // get cell colors
  QColor cellColor, sideColor;

  // nothing to draw
  if (cell.isEmpty() && prevCell.isEmpty()) {
    if (col < 0) {
      TStageObjectId cameraId =
          m_viewer->getXsheet()->getStageObjectTree()->getCurrentCameraId();
      bool isActive =
          cameraId.getIndex() == m_viewer->getXsheet()->getCameraColumnIndex();
      cellColor = (isSelected)
                      ? (isActive ? m_viewer->getSelectedActiveCameraColor()
                                  : m_viewer->getSelectedOtherCameraColor())
                      : (isActive ? m_viewer->getActiveCameraColor()
                                  : m_viewer->getOtherCameraColor());
      cellColor.setAlpha(50);

      // paint cell
      if (o->isVerticalTimeline())
        // Paint the cell edge-to-edge, we use LightLineColor with low opacity
        // to pick up the hue of the cell color to make the separator line more
        // pleasing to the eye.
        p.fillRect(rect.adjusted(0, 0, 0, 1), QBrush(cellColor));
      else
        p.fillRect(rect, QBrush(cellColor));
    }

    // cell mark
    if (markId >= 0) {
      p.setBrush(markColor);
      p.setPen(Qt::NoPen);
      p.drawEllipse(markRect);
      // p.fillRect(rect, QBrush(markColor));
    }

    drawFrameSeparator(p, row, col, true);

    if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
        row == m_viewer->getCurrentRow() &&
        Preferences::instance()->isCurrentTimelineIndicatorEnabled())
      drawCurrentTimeIndicator(p, xy, col);
    return;
  }

  bool heldFrame = (!o->isVerticalTimeline() && sameLevel &&
                    prevCell.m_frameId == cell.m_frameId);

  if (cell.isEmpty()) {  // it means previous is not empty
    // diagonal cross meaning end of level

    drawFrameSeparator(p, row, col, true, heldFrame);
    // cell mark
    if (markId >= 0) {
      p.setBrush(markColor);
      p.setPen(Qt::NoPen);
      p.drawEllipse(markRect);
    }

    QColor levelEndColor = m_viewer->getTextColor();
    levelEndColor.setAlphaF(0.3);
    p.setPen(levelEndColor);
    p.drawLine(rect.topLeft(), rect.bottomRight());
    p.drawLine(rect.topRight(), rect.bottomLeft());

    if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
        row == m_viewer->getCurrentRow() &&
        Preferences::instance()->isCurrentTimelineIndicatorEnabled())
      drawCurrentTimeIndicator(p, xy, col);

    return;
  }

  if (isReference) {
    cellColor = (isSelected) ? m_viewer->getSelectedReferenceColumnColor()
                             : m_viewer->getReferenceColumnColor();
    sideColor = m_viewer->getReferenceColumnBorderColor();
  } else {
    int levelType;
    m_viewer->getCellTypeAndColors(levelType, cellColor, sideColor, cell,
                                   isSelected);
  }

  // check if the level is scanned but not cleanupped
  bool yetToCleanupCell = false;
  if (!cell.isEmpty() && cell.getSimpleLevel()) {
    int frameStatus = cell.getSimpleLevel()->getFrameStatus(cell.m_frameId);
    const int mask  = TXshSimpleLevel::Cleanupped | TXshSimpleLevel::Scanned;
    const int yetToCleanupValue = TXshSimpleLevel::Scanned;
    yetToCleanupCell            = (frameStatus & mask) == yetToCleanupValue;
  }

  // paint cell
  if (o->isVerticalTimeline())
    // Paint the cell edge-to-edge, we use LightLineColor with low opacity to
    // pick up the hue of the cell color to make the separator line more
    // pleasing to the eye.
    p.fillRect(rect.adjusted(0, 0, 0, 1), QBrush(cellColor));
  else
    p.fillRect(rect, QBrush(cellColor));

  if (yetToCleanupCell)  // ORIENTATION: what's this?
  {
    if (o->isVerticalTimeline())
      p.fillRect(rect.adjusted(rect.width() / 2, 0, 0, 0),
                 (isSelected) ? m_viewer->getSelectedFullcolorColumnColor()
                              : m_viewer->getFullcolorColumnColor());
    else
      p.fillRect(rect.adjusted(0, rect.height() / 2, 0, 0),
                 (isSelected) ? m_viewer->getSelectedFullcolorColumnColor()
                              : m_viewer->getFullcolorColumnColor());
  }

  if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
      row == m_viewer->getCurrentRow() &&
      Preferences::instance()->isCurrentTimelineIndicatorEnabled())
    drawCurrentTimeIndicator(p, xy, col);

  drawDragHandle(p, xy, sideColor);

  bool isLastRow = nextCell.isEmpty() ||
                   cell.m_level.getPointer() != nextCell.m_level.getPointer();
  drawEndOfDragHandle(p, isLastRow, xy, cellColor);

  drawLockedDottedLine(p, xsh->getColumn(col)->isLocked(), xy, cellColor);

  drawFrameSeparator(p, row, col, false, heldFrame);

  QRect nameRect = o->rect(PredefinedRect::CELL_NAME).translated(QPoint(x, y));

  if (Preferences::instance()->isShowKeyframesOnXsheetCellAreaEnabled()) {
    TStageObject *pegbar = xsh->getStageObject(m_viewer->getObjectId(col));
    int r0, r1;
    if (pegbar && pegbar->getKeyframeRange(r0, r1))
      nameRect = o->rect(PredefinedRect::CELL_NAME_WITH_KEYFRAME)
                     .translated(QPoint(x, y));
  }

  nameRect.adjust(0, 0, -frameAdj.x(), -frameAdj.y());

  // draw text in red if the file does not exist
  bool isRed          = false;
  TXshSimpleLevel *sl = cell.getSimpleLevel();
  if (sl && !sl->isFid(cell.m_frameId)) isRed = true;
  TXshChildLevel *cl = cell.getChildLevel();
  if (cl && cell.getFrameId().getNumber() - 1 >= cl->getFrameCount())
    isRed = true;
  QColor penColor = isRed        ? QColor(m_viewer->getErrorTextColor())
                    : isSelected ? m_viewer->getSelectedTextColor()
                                 : m_viewer->getTextColor();
  p.setPen(penColor);

  /*
  QString fontName = Preferences::instance()->getInterfaceFont();
  if (fontName == "") {
#ifdef _WIN32
    fontName = "Arial";
#else
    fontName = "Helvetica";
#endif
  }
  static QFont font(fontName, -1, QFont::Normal);
  */
  QFont font = p.font();
  font.setPixelSize(XSHEET_FONT_PX_SIZE);
  p.setFont(font);

  // do not draw frame number under RenameCellField
  if (m_renameCell->isVisible() && m_renameCell->isLocatedAt(row, col)) return;

  // if the same level & same fId with the previous cell,
  // draw continue line
  QString fnum;
  if (sameLevel && prevCell.m_frameId == cell.m_frameId) {
    if (o->isVerticalTimeline()) {
      // not on line marker
      PredefinedLine which =
          Preferences::instance()->isLevelNameOnEachMarkerEnabled()
              ? PredefinedLine::CONTINUE_LEVEL_WITH_NAME
              : PredefinedLine::CONTINUE_LEVEL;

      QLine continueLine = o->line(which).translated(xy);
      continueLine.setP2(QPoint(continueLine.x2(), continueLine.y2()) -
                         frameAdj);

      if (!showLevelName) {
        penColor.setAlphaF(0.5);
        p.setPen(penColor);
      }

      p.drawLine(continueLine);
    }
  }
  // draw frame number
  else {
    if (isSimpleView) {
      // cell mark
      if (markId >= 0) {
        p.setBrush(markColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(markRect);
        p.setPen(penColor);
      }

      if (!o->isVerticalTimeline()) {
        // Lets not draw normal marker if there is a keyframe here
        TStageObject *pegbar = xsh->getStageObject(m_viewer->getObjectId(col));
        if (pegbar->isKeyframe(row)) return;
      }

      drawFrameMarker(p, QPoint(x, y), (isRed ? Qt::red : Qt::black));
      return;
    }

    TFrameId fid = cell.m_frameId;

    // convert the last one digit of the frame number to alphabet
    // Ex.  12 -> 1B    21 -> 2A   30 -> 3
    if (Preferences::instance()->isShowFrameNumberWithLettersEnabled())
      fnum = m_viewer->getFrameNumberWithLetters(fid.getNumber());
    else {
      QString frameNumber("");
      // set number
      if (fid.getNumber() >= 0) frameNumber = QString::number(fid.getNumber());
      // add letter
      if (!fid.getLetter().isEmpty()) frameNumber += fid.getLetter();
      fnum = frameNumber;
    }

    int alignFlag =
        ((showLevelName) ? Qt::AlignRight | Qt::AlignBottom : Qt::AlignCenter);
    p.drawText(nameRect, alignFlag, fnum);
  }

  // cell mark
  if (markId >= 0) {
    p.setBrush(markColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(markRect);
    p.setPen(penColor);
  }

  int distance, offset, secDistance;
  TApp::instance()->getCurrentScene()->getScene()->getProperties()->getMarkers(
      distance, offset, secDistance);
  bool isAfterMarkers =
      distance > 0 && ((row - offset) % distance) == 0 && row != 0;

  // draw level name
  if (showLevelName &&
      (!sameLevel ||
       (isAfterMarkers && !isSimpleView &&
        Preferences::instance()->isLevelNameOnEachMarkerEnabled()))) {
    std::wstring levelName = cell.m_level->getName();
    QString text           = QString::fromStdWString(levelName);
    QFontMetrics fm(font);
    QString elidaName =
        elideText(text, fm, nameRect.width() - fm.width(fnum), QString("~"));
    p.drawText(nameRect, Qt::AlignLeft | Qt::AlignBottom, elidaName);
  }
}

//-----------------------------------------------------------------------------

void CellArea::drawSoundTextCell(QPainter &p, int row, int col) {
  const Orientation *o = m_viewer->orientation();
  TXsheet *xsh         = m_viewer->getXsheet();
  TXshCell cell        = xsh->getCell(row, col);
  TXshCell prevCell;

  TCellSelection *cellSelection     = m_viewer->getCellSelection();
  TColumnSelection *columnSelection = m_viewer->getColumnSelection();
  bool isSelected                   = cellSelection->isCellSelected(row, col) ||
                    columnSelection->isColumnSelected(col);

  if (row > 0)
    prevCell = xsh->getCell(row - 1, col);  // cell in previous frame
                                            // nothing to draw

  bool sameLevel = prevCell.m_level.getPointer() == cell.m_level.getPointer();

  QPoint xy = m_viewer->positionToXY(CellPosition(row, col));
  int x     = xy.x();
  int y     = xy.y();
  if (row == 0) {
    if (o->isVerticalTimeline())
      xy.setY(xy.y() + 1);
    else
      xy.setX(xy.x() + 1);
  }

  TXshCell nextCell = xsh->getCell(row + 1, col);
  QPoint frameAdj   = m_viewer->getFrameZoomAdjustment();
  QRect cellRect    = o->rect(PredefinedRect::CELL).translated(QPoint(x, y));
  cellRect.adjust(0, 0, -frameAdj.x(), -frameAdj.y());
  QRect rect = cellRect.adjusted(
      1, 1,
      (!m_viewer->orientation()->isVerticalTimeline() && !nextCell.isEmpty()
           ? 2
           : 0),
      0);
  int markId = xsh->getColumn(col)->getCellColumn()->getCellMark(row);
  QColor markColor;
  if (markId >= 0) {
    TPixel32 col = TApp::instance()
                       ->getCurrentScene()
                       ->getScene()
                       ->getProperties()
                       ->getCellMark(markId)
                       .color;
    markColor = QColor(col.r, col.g, col.b, 196);  // semi transparent
  }
  QRect markRect =
      o->rect(PredefinedRect::CELL_MARK_AREA)
          .adjusted(0, -std::round(double(frameAdj.y()) * 0.1), -frameAdj.y(),
                    -std::round(double(frameAdj.y()) * 0.9))
          .translated(xy);
  if (markRect.right() > rect.right()) markRect.setRight(rect.right());

  if (cell.isEmpty() && prevCell.isEmpty()) {
    drawFrameSeparator(p, row, col, true);
    if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
        row == m_viewer->getCurrentRow() &&
        Preferences::instance()->isCurrentTimelineIndicatorEnabled())
      drawCurrentTimeIndicator(p, xy, col);

    // only draw mark
    if (markId >= 0) {
      p.setBrush(markColor);
      p.setPen(Qt::NoPen);
      p.drawEllipse(markRect);
    }
    return;
  }

  bool heldFrame = (!o->isVerticalTimeline() && sameLevel &&
                    prevCell.m_frameId == cell.m_frameId);

  if (cell.isEmpty()) {  // diagonal cross meaning end of level

    drawFrameSeparator(p, row, col, false, heldFrame);

    QColor levelEndColor = m_viewer->getTextColor();
    levelEndColor.setAlphaF(0.3);
    p.setPen(levelEndColor);
    p.drawLine(rect.topLeft(), rect.bottomRight());
    p.drawLine(rect.topRight(), rect.bottomLeft());

    // only draw mark
    if (markId >= 0) {
      p.setBrush(markColor);
      p.setPen(Qt::NoPen);
      p.drawEllipse(markRect);
    }
    if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
        row == m_viewer->getCurrentRow() &&
        Preferences::instance()->isCurrentTimelineIndicatorEnabled())
      drawCurrentTimeIndicator(p, xy, col);

    return;
  }

  int levelType;
  QColor cellColor, sideColor;
  m_viewer->getCellTypeAndColors(levelType, cellColor, sideColor, cell,
                                 isSelected);

  // paint cell
  if (o->isVerticalTimeline())
    // Paint the cell edge-to-edge, we use LightLineColor with low opacity to
    // pick up the hue of the cell color to make the separator line more
    // pleasing to the eye.
    p.fillRect(rect.adjusted(0, 0, 0, 1), QBrush(cellColor));
  else
    p.fillRect(rect, QBrush(cellColor));

  if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
      row == m_viewer->getCurrentRow() &&
      Preferences::instance()->isCurrentTimelineIndicatorEnabled())
    drawCurrentTimeIndicator(p, xy, col);

  drawDragHandle(p, xy, sideColor);

  bool isLastRow = nextCell.isEmpty() ||
                   cell.m_level.getPointer() != nextCell.m_level.getPointer();
  drawEndOfDragHandle(p, isLastRow, xy, cellColor);

  drawLockedDottedLine(p, xsh->getColumn(col)->isLocked(), xy, cellColor);

  drawFrameSeparator(p, row, col, false, heldFrame);

  // cell mark
  if (markId >= 0) {
    p.setBrush(markColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(markRect);
  }

  TFrameId fid = cell.m_frameId;
  if (fid.getNumber() - 1 < 0) return;

  p.setPen(Qt::black);
  QRect nameRect = o->rect(PredefinedRect::CELL_NAME).translated(QPoint(x, y));
  nameRect.adjust(0, 0, -frameAdj.x(), -frameAdj.y());

  // il nome va scritto se e' diverso dalla cella precedente oppure se
  // siamo su una marker line
  QString fontName = Preferences::instance()->getInterfaceFont();
  if (fontName == "") {
#ifdef _WIN32
    fontName = "Arial";
#else
    fontName = "Helvetica";
#endif
  }
  static QFont font(fontName, -1, QFont::Normal);
  font.setPixelSize(XSHEET_FONT_PX_SIZE);
  p.setFont(font);

  // if the same level & same fId with the previous cell,
  // draw continue line
  if (sameLevel && prevCell.m_frameId == cell.m_frameId) {
    if (!o->isVerticalTimeline()) return;
    // not on line marker
    PredefinedLine which =
        Preferences::instance()->isLevelNameOnEachMarkerEnabled()
            ? PredefinedLine::CONTINUE_LEVEL_WITH_NAME
            : PredefinedLine::CONTINUE_LEVEL;
    QLine continueLine = o->line(which).translated(xy);
    continueLine.setP2(QPoint(continueLine.x2(), continueLine.y2()) - frameAdj);
    p.drawLine(continueLine);
  }

  QString text =
      cell.getSoundTextLevel()->getFrameText(cell.m_frameId.getNumber() - 1);

  QFontMetrics metric(font);

  int charWidth = metric.width(text, 1);
  if ((charWidth * 2) > nameRect.width()) nameRect.adjust(-2, 0, 4, 0);

  QString elidaName = elideText(text, metric, nameRect.width(), "~");

  if (!sameLevel || prevCell.m_frameId != cell.m_frameId)
    p.drawText(nameRect, Qt::AlignLeft | Qt::AlignBottom, elidaName);
}

//-----------------------------------------------------------------------------

void CellArea::drawSoundTextColumn(QPainter &p, int r0, int r1, int col) {
  const Orientation *o = m_viewer->orientation();
  TXsheet *xsh         = m_viewer->getXsheet();

  struct CellInfo {
    int row;
    QRect rect;
    QPoint xy;
    int markId;
    QColor markColor;
    QRect markRect;
    QRect nameRect;
  };

  auto getCellInfo = [&](int r) {
    CellInfo ret;
    ret.row         = r;
    ret.xy          = m_viewer->positionToXY(CellPosition(r, col));
    QPoint frameAdj = m_viewer->getFrameZoomAdjustment();
    QRect cellRect  = o->rect(PredefinedRect::CELL).translated(ret.xy);
    cellRect.adjust(0, 0, -frameAdj.x(), -frameAdj.y());
    ret.rect = cellRect.adjusted(1, 1, 0, 1);

    ret.markId = xsh->getColumn(col)->getCellColumn()->getCellMark(r);
    if (ret.markId >= 0) {
      TPixel32 col = TApp::instance()
                         ->getCurrentScene()
                         ->getScene()
                         ->getProperties()
                         ->getCellMark(ret.markId)
                         .color;
      ret.markColor = QColor(col.r, col.g, col.b, 196);  // semi transparent
      ret.markRect =
          o->rect(PredefinedRect::CELL_MARK_AREA)
              .adjusted(0, -std::round(double(frameAdj.y()) * 0.1),
                        -frameAdj.y(), -std::round(double(frameAdj.y()) * 0.9))
              .translated(ret.xy);
      if (ret.markRect.right() > ret.rect.right())
        ret.markRect.setRight(ret.rect.right());
    }
    ret.nameRect = o->rect(PredefinedRect::CELL_NAME)
                       .translated(ret.xy)
                       .adjusted(0, 0, -frameAdj.x(), -frameAdj.y());

    return ret;
  };

  QString fontName = Preferences::instance()->getInterfaceFont();
  if (fontName == "") {
#ifdef _WIN32
    fontName = "Arial";
#else
    fontName = "Helvetica";
#endif
  }
  static QFont font(fontName, -1, QFont::Normal);
  font.setPixelSize(XSHEET_FONT_PX_SIZE);
  static QFont largeFont(font);
  largeFont.setPixelSize(XSHEET_FONT_PX_SIZE * 13 / 10);
  QFontMetrics fm(font);
  int heightThres = QFontMetrics(largeFont).height();

  int rStart = r0;
  int rEnd   = r1;
  // obtain top row of the fist note block
  if (!xsh->getCell(r0, col).isEmpty()) {
    while (rStart > 0 &&
           xsh->getCell(rStart - 1, col) == xsh->getCell(r0, col)) {
      rStart--;
    }
  }
  // obtain bottom row of the last note block
  if (!xsh->getCell(r1, col).isEmpty()) {
    while (xsh->getCell(rEnd + 1, col) == xsh->getCell(r1, col)) {
      rEnd++;
    }
  }

  QColor cellColor         = m_viewer->getSoundTextColumnColor();
  QColor selectedCellColor = m_viewer->getSelectedSoundTextColumnColor();
  QColor sideColor         = m_viewer->getSoundTextColumnBorderColor();

  TCellSelection *cellSelection     = m_viewer->getCellSelection();
  TColumnSelection *columnSelection = m_viewer->getColumnSelection();
  bool isColSelected                = columnSelection->isColumnSelected(col);

  // for each row
  for (int row = rStart; row <= rEnd; row++) {
    TXshCell cell = xsh->getCell(row, col);

    // if the cell is empty
    if (cell.isEmpty()) {
      CellInfo info = getCellInfo(row);
      drawFrameSeparator(p, row, col, true);
      // draw X shape after the occupied cell
      TXshCell prevCell;
      if (row > 0) prevCell = xsh->getCell(row - 1, col);
      if (!prevCell.isEmpty()) {
        QColor levelEndColor = m_viewer->getTextColor();
        levelEndColor.setAlphaF(0.3);
        p.setPen(levelEndColor);
        p.drawLine(info.rect.topLeft(), info.rect.bottomRight());
        p.drawLine(info.rect.topRight(), info.rect.bottomLeft());
      }
      if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
          row == m_viewer->getCurrentRow() &&
          Preferences::instance()->isCurrentTimelineIndicatorEnabled())
        drawCurrentTimeIndicator(p, info.xy, col);
      // draw mark
      if (info.markId >= 0) {
        p.setBrush(info.markColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(info.markRect);
      }
      continue;
    }

    //---- if the current cell is occupied

    QList<CellInfo> infoList;
    // check how long the same content continues
    int rowTo = row;
    infoList.append(getCellInfo(row));
    while (xsh->getCell(rowTo + 1, col) == cell) {
      rowTo++;
      infoList.append(getCellInfo(rowTo));
    }

    // for each cell block with the same content

    // paint background and other stuffs
    for (auto info : infoList) {
      bool isSelected =
          isColSelected || cellSelection->isCellSelected(info.row, col);
      QColor tmpCellColor = (isSelected) ? selectedCellColor : cellColor;
      if (!o->isVerticalTimeline() && info.row != rowTo)
        info.rect.adjust(0, 0, 2, 0);
      p.fillRect(info.rect, QBrush(tmpCellColor));

      if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
          info.row == m_viewer->getCurrentRow() &&
          Preferences::instance()->isCurrentTimelineIndicatorEnabled())
        drawCurrentTimeIndicator(p, info.xy, col);

      drawDragHandle(p, info.xy, sideColor);
      drawEndOfDragHandle(p, info.row == rowTo, info.xy, tmpCellColor);
      drawLockedDottedLine(p, xsh->getColumn(col)->isLocked(), info.xy,
                           tmpCellColor);

      bool heldFrame = (!o->isVerticalTimeline() && info.row != row);
      drawFrameSeparator(p, info.row, col, false, heldFrame);
      // draw mark
      if (info.markId >= 0) {
        p.setBrush(info.markColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(info.markRect);
      }
    }

    // draw text from here

    QString text =
        cell.getSoundTextLevel()->getFrameText(cell.m_frameId.getNumber() - 1);
    if (text.isEmpty()) {
      // advance the current row
      row = rowTo;
      continue;
    }
    int textCount = text.count();

    p.setPen(Qt::black);
    // Vertical case
    if (o->isVerticalTimeline()) {
      int lettersPerChunk =
          (int)std::ceil((double)textCount / (double)(rowTo - row + 1));
      int chunkCount =
          (int)std::ceil((double)textCount / (double)(lettersPerChunk));
      bool isChunkOverflow = false;
      for (int c = 0; c < chunkCount; c++) {
        int chunkWidth =
            fm.boundingRect(text.mid(c * lettersPerChunk, lettersPerChunk))
                .width();
        if (chunkWidth > infoList.front().nameRect.width()) {
          isChunkOverflow = true;
          break;
        }
      }
      // if any chunk overflows the cell width
      if (isChunkOverflow) {
        p.setFont(font);
        // arrange text from the top cell and elide at the last cell
        int textPos = 0;
        for (auto info : infoList) {
          // add letter and check if the text can be inside the cell
          int len = 1;
          while (textPos + len < textCount &&
                 fm.boundingRect(text.mid(textPos, len + 1)).width() <=
                     info.nameRect.width()) {
            len++;
          }
          // elide text at the last row
          QString curText =
              (info.row == rowTo)
                  ? elideText(text.mid(textPos), fm, info.nameRect.width(), "~")
                  : text.mid(textPos, len);

          p.drawText(info.nameRect, Qt::AlignCenter, curText);
          textPos += len;
          if (textPos >= textCount) break;
        }
      }
      // if all text chunks can be inside the cells
      else {
        // unite the cell rects and divide by the amount of chunks
        QRect unitedRect =
            infoList.front().nameRect.united(infoList.last().nameRect);
        // check if the large font is available
        if (lettersPerChunk == 1 &&
            unitedRect.height() / textCount > heightThres)
          p.setFont(largeFont);
        else
          p.setFont(font);
        // draw text
        for (int c = 0; c < chunkCount; c++) {
          int y0 = unitedRect.top() + unitedRect.height() * c / chunkCount;
          int y1 =
              unitedRect.top() + unitedRect.height() * (c + 1) / chunkCount;
          QRect tmpRect(unitedRect.left(), y0, unitedRect.width(), y1 - y0 + 1);
          p.drawText(tmpRect, Qt::AlignCenter,
                     text.mid(c * lettersPerChunk, lettersPerChunk));
        }
      }
    }
    // Horizontal case
    else {
      p.setFont(font);
      // unite the cell rects
      QRect unitedRect =
          infoList.front().nameRect.united(infoList.last().nameRect);
      int extraWidth = unitedRect.width() - fm.boundingRect(text).width();
      if (extraWidth >= 0) {
        int margin = extraWidth / (2 * textCount);
        // Qt::TextJustificationForced flag is needed to make Qt::AlignJustify
        // to work on the single-line text
        p.drawText(
            unitedRect.adjusted(margin, 0, -margin, 0),
            Qt::TextJustificationForced | Qt::AlignJustify | Qt::AlignVCenter,
            text);
      } else {
        QString elided = elideText(text, fm, unitedRect.width(), "~");
        p.drawText(unitedRect, Qt::AlignLeft | Qt::AlignVCenter, elided);
      }
    }

    // advance the current row
    row = rowTo;
  }
}

//-----------------------------------------------------------------------------

void CellArea::drawPaletteCell(QPainter &p, int row, int col,
                               bool isReference) {
  const Orientation *o = m_viewer->orientation();

  TXsheet *xsh  = m_viewer->getXsheet();
  TXshCell cell = xsh->getCell(row, col);
  TXshCell prevCell;

  TCellSelection *cellSelection = m_viewer->getCellSelection();
  bool isSelected               = cellSelection->isCellSelected(row, col);

  if (row > 0) prevCell = xsh->getCell(row - 1, col);
  TXshCell nextCell = xsh->getCell(row + 1, col);

  bool sameLevel = prevCell.m_level.getPointer() == cell.m_level.getPointer();

  bool isRed           = false;
  TXshPaletteLevel *pl = cell.getPaletteLevel();
  if (pl && !pl->getPalette()) isRed = true;

  bool isSimpleView = m_viewer->getFrameZoomFactor() <=
                      o->dimension(PredefinedDimension::SCALE_THRESHOLD);

  QPoint xy = m_viewer->positionToXY(CellPosition(row, col));
  int x     = xy.x();
  int y     = xy.y();
  if (row == 0) {
    if (o->isVerticalTimeline())
      xy.setY(xy.y() + 1);
    else
      xy.setX(xy.x() + 1);
  }

  QPoint frameAdj = m_viewer->getFrameZoomAdjustment();
  QRect cellRect  = o->rect(PredefinedRect::CELL).translated(QPoint(x, y));
  cellRect.adjust(0, 0, -frameAdj.x(), -frameAdj.y());
  QRect rect = cellRect.adjusted(
      1, 1,
      (!m_viewer->orientation()->isVerticalTimeline() && !nextCell.isEmpty()
           ? 2
           : 0),
      0);
  int markId = xsh->getColumn(col)->getCellColumn()->getCellMark(row);
  QColor markColor;
  if (markId >= 0) {
    TPixel32 col = TApp::instance()
                       ->getCurrentScene()
                       ->getScene()
                       ->getProperties()
                       ->getCellMark(markId)
                       .color;
    markColor = QColor(col.r, col.g, col.b, 196);  // semi transparent
  }
  QRect markRect =
      o->rect(PredefinedRect::CELL_MARK_AREA)
          .adjusted(0, -std::round(double(frameAdj.y()) * 0.1), -frameAdj.y(),
                    -std::round(double(frameAdj.y()) * 0.9))
          .translated(xy);
  if (!isSimpleView || !o->isVerticalTimeline())
    markRect.moveCenter(cellRect.center());
  if (markRect.right() > rect.right()) markRect.setRight(rect.right());

  if (cell.isEmpty() && prevCell.isEmpty()) {
    drawFrameSeparator(p, row, col, true);
    if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
        row == m_viewer->getCurrentRow() &&
        Preferences::instance()->isCurrentTimelineIndicatorEnabled())
      drawCurrentTimeIndicator(p, xy, col);

    // only draw mark
    if (markId >= 0) {
      p.setBrush(markColor);
      p.setPen(Qt::NoPen);
      p.drawEllipse(markRect);
    }
    return;
  }

  int distance, offset, secDistance;
  TApp::instance()->getCurrentScene()->getScene()->getProperties()->getMarkers(
      distance, offset, secDistance);

  bool isAfterMarkers =
      distance > 0 && ((row - offset) % distance) == 0 && row != 0;

  bool heldFrame = (!o->isVerticalTimeline() && !isAfterMarkers && sameLevel &&
                    prevCell.m_frameId == cell.m_frameId);

  if (cell.isEmpty()) {  // this means the former is not empty

    drawFrameSeparator(p, row, col, false, heldFrame);

    QColor levelEndColor = m_viewer->getTextColor();
    levelEndColor.setAlphaF(0.3);
    p.setPen(levelEndColor);
    p.drawLine(rect.topLeft(), rect.bottomRight());
    p.drawLine(rect.topRight(), rect.bottomLeft());

    // only draw mark
    if (markId >= 0) {
      p.setBrush(markColor);
      p.setPen(Qt::NoPen);
      p.drawEllipse(markRect);
    }
    if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
        row == m_viewer->getCurrentRow() &&
        Preferences::instance()->isCurrentTimelineIndicatorEnabled())
      drawCurrentTimeIndicator(p, xy, col);

    return;
  }

  QColor cellColor, sideColor;
  if (isReference) {
    cellColor = (isSelected) ? m_viewer->getSelectedReferenceColumnColor()
                             : m_viewer->getReferenceColumnColor();
    sideColor = m_viewer->getReferenceColumnBorderColor();
  } else {
    int levelType;
    m_viewer->getCellTypeAndColors(levelType, cellColor, sideColor, cell,
                                   isSelected);
  }

  // paint cell
  if (o->isVerticalTimeline())
    // Paint the cell edge-to-edge, we use LightLineColor with low opacity to
    // pick up the hue of the cell color to make the separator line more
    // pleasing to the eye.
    p.fillRect(rect.adjusted(0, 0, 0, 1), QBrush(cellColor));
  else
    p.fillRect(rect, QBrush(cellColor));

  if (TApp::instance()->getCurrentFrame()->isEditingScene() &&
      row == m_viewer->getCurrentRow() &&
      Preferences::instance()->isCurrentTimelineIndicatorEnabled())
    drawCurrentTimeIndicator(p, xy, col);

  drawDragHandle(p, xy, sideColor);
  bool isLastRow = nextCell.isEmpty() ||
                   cell.m_level.getPointer() != nextCell.m_level.getPointer();
  drawEndOfDragHandle(p, isLastRow, xy, cellColor);
  drawLockedDottedLine(p, xsh->getColumn(col)->isLocked(), xy, cellColor);

  drawFrameSeparator(p, row, col, false, heldFrame);

  if (sameLevel && prevCell.m_frameId == cell.m_frameId &&
      !isAfterMarkers) {  // cell equal to previous one (not on marker line):
                          // do not write anything and draw a vertical line
    if (o->isVerticalTimeline()) {
      QPen oldPen = p.pen();
      p.setPen(QPen(m_viewer->getTextColor(), 1));
      QLine continueLine =
          o->line(PredefinedLine::CONTINUE_LEVEL).translated(xy);
      continueLine.setP2(QPoint(continueLine.x2(), continueLine.y2()) -
                         frameAdj);
      p.drawLine(continueLine);
      p.setPen(oldPen);
    }
  } else {
    if (isSimpleView) {
      // cell mark
      if (markId >= 0) {
        p.setBrush(markColor);
        p.setPen(Qt::NoPen);
        p.drawEllipse(markRect);
      }
      if (!o->isVerticalTimeline()) {
        // Lets not draw normal marker if there is a keyframe here
        TStageObject *pegbar = xsh->getStageObject(m_viewer->getObjectId(col));
        if (pegbar->isKeyframe(row)) return;
      }
      drawFrameMarker(p, QPoint(x, y), (isRed ? Qt::red : Qt::black));
      return;
    }

    TFrameId fid = cell.m_frameId;

    std::wstring levelName = cell.m_level->getName();
    // QString frameNumber("");
    // if (fid.getNumber() > 0) frameNumber = QString::number(fid.getNumber());
    // if (fid.getLetter() != 0) frameNumber += fid.getLetter();

    QRect nameRect =
        o->rect(PredefinedRect::CELL_NAME).translated(QPoint(x, y));

    if (Preferences::instance()->isShowKeyframesOnXsheetCellAreaEnabled()) {
      TStageObject *pegbar = xsh->getStageObject(m_viewer->getObjectId(col));
      int r0, r1;
      if (pegbar && pegbar->getKeyframeRange(r0, r1))
        nameRect = o->rect(PredefinedRect::CELL_NAME_WITH_KEYFRAME)
                       .translated(QPoint(x, y));
    }

    nameRect.adjust(0, 0, -frameAdj.x(), -frameAdj.y());
    QColor penColor = isRed ? QColor(m_viewer->getErrorTextColor())
                            : m_viewer->getTextColor();
    p.setPen(penColor);
    // il nome va scritto se e' diverso dalla cella precedente oppure se
    // siamo su una marker line
    QString fontName = Preferences::instance()->getInterfaceFont();
    if (fontName == "") {
#ifdef _WIN32
      fontName = "Arial";
#else
      fontName = "Helvetica";
#endif
    }
    static QFont font(fontName, -1, QFont::Normal);
    font.setPixelSize(XSHEET_FONT_PX_SIZE);
    p.setFont(font);
    QFontMetrics fm(font);

    // il numero va scritto se e' diverso dal precedente oppure se il livello
    // e' diverso dal precedente
    QString numberStr;
    if (!sameLevel || prevCell.m_frameId != cell.m_frameId) {
      // convert the last one digit of the frame number to alphabet
      // Ex.  12 -> 1B    21 -> 2A   30 -> 3
      if (Preferences::instance()->isShowFrameNumberWithLettersEnabled()) {
        numberStr = m_viewer->getFrameNumberWithLetters(fid.getNumber());
        p.drawText(nameRect, Qt::AlignRight | Qt::AlignBottom, numberStr);
      } else {
        QString frameNumber("");
        // set number
        if (fid.getNumber() > 0) frameNumber = QString::number(fid.getNumber());
        // add letter
        if (!fid.getLetter().isEmpty()) frameNumber += fid.getLetter();
        p.drawText(nameRect, Qt::AlignRight | Qt::AlignBottom, frameNumber);
      }
    }

    QString text      = QString::fromStdWString(levelName);
    QString elidaName = elideText(
        text, fm, nameRect.width() - fm.width(numberStr) - 2, QString("~"));

    if (!sameLevel || isAfterMarkers)
      p.drawText(nameRect, Qt::AlignLeft | Qt::AlignBottom, elidaName);
  }

  // cell mark
  if (markId >= 0) {
    p.setBrush(markColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(markRect);
  }
}

//-----------------------------------------------------------------------------

void CellArea::drawKeyframe(QPainter &p, const QRect toBeUpdated) {
  const Orientation *o = m_viewer->orientation();
  int r0, r1, c0, c1;  // range of visible rows and columns
  CellRange visible    = m_viewer->xyRectToRange(toBeUpdated);
  QColor keyFrameColor = Qt::white, outline = Qt::black;
  r0 = visible.from().frame();
  r1 = visible.to().frame();
  c0 = visible.from().layer();
  c1 = visible.to().layer();

  static QPixmap selectedKey = svgToPixmap(":Resources/selected_key.svg");
  static QPixmap key         = svgToPixmap(":Resources/key.svg");
  QPoint frameAdj            = m_viewer->getFrameZoomAdjustment();
  const QRect &keyRect =
      o->rect(PredefinedRect::KEY_ICON).translated(-frameAdj / 2);

  TXsheet *xsh         = m_viewer->getXsheet();
  ColumnFan *columnFan = xsh->getColumnFan(o);
  int col;
  for (col = c0; col <= c1; col++) {
    if (!columnFan->isActive(col)) continue;
    int layerAxis = m_viewer->columnToLayerAxis(col);

    TStageObject *pegbar = xsh->getStageObject(m_viewer->getObjectId(col));
    if (!pegbar) return;

    int row0, row1;
    bool emptyKeyframe = !pegbar->getKeyframeRange(row0, row1);
    if (emptyKeyframe) continue;

    bool emptyKeyframeRange = row0 >= row1;
    int row;
    row0 = std::max(row0, r0);
    row1 = std::min(row1, r1);

    QRect tmpKeyRect = (col >= 0) ? keyRect
                                  : o->rect(PredefinedRect::CAMERA_KEY_ICON)
                                        .translated(-frameAdj / 2);

    /*- first, draw key segments -*/
    p.setPen(m_viewer->getTextColor());
    int line_layerAxis = layerAxis + o->layerSide(tmpKeyRect).middle();
    for (row = row0; row <= row1; row++) {
      int segmentRow0, segmentRow1;
      double ease0, ease1;
      if (pegbar->getKeyframeSpan(row, segmentRow0, ease0, segmentRow1,
                                  ease1)) {
        drawKeyframeLine(p, col, NumberRange(segmentRow0, segmentRow1));

        if (segmentRow1 - segmentRow0 >
            4) {  // only show if distance more than 4 frames
          int handleRow0, handleRow1;
          if (getEaseHandles(segmentRow0, segmentRow1, ease0, ease1, handleRow0,
                             handleRow1)) {
            QPoint topLeft =
                m_viewer->positionToXY(CellPosition(handleRow0, col));
            m_viewer->drawPredefinedPath(
                p, PredefinedPath::BEGIN_EASE_TRIANGLE,
                tmpKeyRect.translated(topLeft).center(), keyFrameColor,
                outline);

            topLeft = m_viewer->positionToXY(CellPosition(handleRow1, col));
            m_viewer->drawPredefinedPath(
                p, PredefinedPath::END_EASE_TRIANGLE,
                tmpKeyRect.translated(topLeft).center(), keyFrameColor,
                outline);
          }
        }
        // skip to next segment
        row = segmentRow1 - 1;
      } else if (pegbar->isKeyframe(row) && pegbar->isKeyframe(row + 1)) {
        // 2 keyframes in a row - connect with a short line
        drawKeyframeLine(p, col, NumberRange(row, row + 1));
      }
    }

    /*- then draw keyframe pixmaps -*/
    int icon_layerAxis = line_layerAxis - 5;
    for (row = row0; row <= row1; row++) {
      p.setPen(m_viewer->getTextColor());
      if (pegbar->isKeyframe(row)) {
        QPoint xy     = m_viewer->positionToXY(CellPosition(row, col));
        QPoint target = tmpKeyRect.translated(xy).topLeft();
        if (m_viewer->getFrameZoomFactor() <=
            o->dimension(PredefinedDimension::SCALE_THRESHOLD)) {
          QColor color = Qt::white;
          int x        = xy.x();
          int y        = xy.y();
          if (row == 0) {
            if (o->isVerticalTimeline())
              xy.setY(xy.y() + 1);
            else
              xy.setX(xy.x() + 1);
          }

          if (m_viewer->getKeyframeSelection() &&
              m_viewer->getKeyframeSelection()->isSelected(row, col))
            color = QColor(85, 157, 255);

          drawFrameMarker(p, QPoint(x, y), color, true, col < 0);

        } else if (m_viewer->getKeyframeSelection() &&
                   m_viewer->getKeyframeSelection()->isSelected(row, col)) {
          // keyframe selected
          p.drawPixmap(target, selectedKey);
        } else {
          // keyframe not selected
          p.drawPixmap(target, key);
        }
      }
    }

    int icon_frameAxis = m_viewer->rowToFrameAxis(row1 + 1);
    if (!emptyKeyframeRange && row0 <= row1 + 1) {
      // there's just a keyframe
      // drawing loop button
      p.setBrush(Qt::white);
      p.setPen(Qt::black);
      QPoint target = o->frameLayerToXY(icon_frameAxis, icon_layerAxis);
      p.drawRect(QRect(target, QSize(10, 10)));
      p.setBrush(Qt::NoBrush);
      // drawing the bottom edge (rounded)
      p.drawLine(target + QPoint(1, 10), target + QPoint(9, 10));
      p.setPen(Qt::white);
      p.drawLine(target + QPoint(3, 10), target + QPoint(7, 10));
      p.setPen(Qt::black);
      p.drawLine(target + QPoint(3, 11), target + QPoint(7, 11));

      // drawing the arrow
      p.drawArc(QRect(target + QPoint(2, 3), QSize(6, 6)), 180 * 16, 270 * 16);
      p.drawLine(target + QPoint(5, 2), target + QPoint(5, 5));
      p.drawLine(target + QPoint(5, 2), target + QPoint(8, 2));
    }
    if (pegbar->isCycleEnabled()) {
      // the row zigzag below the button
      int ymax = m_viewer->rowToFrameAxis(r1 + 1);
      int qy   = icon_frameAxis + 12;
      int zig  = 2;
      int qx   = icon_layerAxis + 5;
      p.setPen(m_viewer->getTextColor());
      p.drawLine(o->frameLayerToXY(qy, qx),
                 o->frameLayerToXY(qy + zig, qx - zig));
      while (qy < ymax) {
        p.drawLine(o->frameLayerToXY(qy + zig, qx - zig),
                   o->frameLayerToXY(qy + 3 * zig, qx + zig));
        p.drawLine(o->frameLayerToXY(qy + 3 * zig, qx + zig),
                   o->frameLayerToXY(qy + 5 * zig, qx - zig));
        qy += 4 * zig;
      }
    }
  }
}

void CellArea::drawKeyframeLine(QPainter &p, int col,
                                const NumberRange &rows) const {
  QPoint frameAdj      = m_viewer->getFrameZoomAdjustment();
  const QRect &keyRect = m_viewer->orientation()
                             ->rect((col < 0) ? PredefinedRect::CAMERA_KEY_ICON
                                              : PredefinedRect::KEY_ICON)
                             .translated(-frameAdj / 2);
  QPoint begin =
      keyRect.center() + m_viewer->positionToXY(CellPosition(rows.from(), col));
  QPoint end =
      keyRect.center() + m_viewer->positionToXY(CellPosition(rows.to(), col));
  p.setPen(m_viewer->getTextColor());
  p.drawLine(QLine(begin, end));
}

//-----------------------------------------------------------------------------

void CellArea::drawNotes(QPainter &p, const QRect toBeUpdated) {
  CellRange visible = m_viewer->xyRectToRange(toBeUpdated);
  int r0, r1, c0, c1;  // range of visible rows and columns
  r0 = visible.from().frame();
  r1 = visible.to().frame();
  c0 = visible.from().layer();
  c1 = visible.to().layer();

  TXsheet *xsh = m_viewer->getXsheet();
  if (!xsh) return;
  TXshNoteSet *notes = xsh->getNotes();
  int notesCount     = notes->getCount();
  int i;
  for (i = 0; i < notesCount; i++) {
    QList<NoteWidget *> noteWidgets = m_viewer->getNotesWidget();
    int widgetCount                 = noteWidgets.size();
    NoteWidget *noteWidget          = 0;
    if (i < widgetCount)
      noteWidget = noteWidgets.at(i);
    else {
      noteWidget = new NoteWidget(m_viewer, i);
      m_viewer->addNoteWidget(noteWidget);
    }
    if (!noteWidget) continue;
    int r = notes->getNoteRow(i);
    int c = notes->getNoteCol(i);
    if (r < r0 || r > r1 || c < c0 || c > c1) continue;
    QPoint xy   = m_viewer->positionToXY(CellPosition(r, c));
    TPointD pos = notes->getNotePos(i) + TPointD(xy.x(), xy.y());
    noteWidget->paint(&p, QPoint(pos.x, pos.y),
                      i == m_viewer->getCurrentNoteIndex());
  }
}

//-----------------------------------------------------------------------------

bool CellArea::getEaseHandles(int r0, int r1, double e0, double e1, int &rh0,
                              int &rh1) {
  if (r1 <= r0 + 4) {  // ... what?
    rh0 = r0;
    rh1 = r1;
    return false;
  }
  if (e0 < 0 || e1 < 0) {
    rh0 = r0;
    rh1 = r1;
    return false;
  }
  if (e0 <= 0 && e1 <= 0) {
    rh0 = r0 + 1;
    rh1 = r1 - 1;
  } else if (e0 <= 0) {
    rh0   = r0 + 1;
    int a = rh0 + 1;
    int b = r1 - 2;
    assert(a <= b);
    rh1 = tcrop((int)(r1 - e1 + 0.5), a, b);
  } else if (e1 <= 0) {
    rh1   = r1 - 1;
    int b = rh1 - 1;
    int a = r0 + 2;
    assert(a <= b);
    rh0 = tcrop((int)(r0 + e0 + 0.5), a, b);
  } else {
    int m = tfloor(0.5 * (r0 + e0 + r1 - e1));
    m     = tcrop(m, r0 + 2, r1 - 2);
    int a = r0 + 2;
    int b = std::min(m, r1 - 3);
    assert(a <= b);
    rh0 = tcrop((int)(r0 + e0 + 0.5), a, b);
    a   = rh0 + 1;
    b   = r1 - 2;
    assert(a <= b);
    rh1 = tcrop((int)(r1 - e1 + 0.5), a, b);
  }
  return true;
}

//-----------------------------------------------------------------------------

void CellArea::paintEvent(QPaintEvent *event) {
  QRect toBeUpdated = event->rect();

  QPainter p(this);
  p.setClipRect(toBeUpdated);

  p.fillRect(toBeUpdated, QBrush(m_viewer->getEmptyCellColor()));

  drawCells(p, toBeUpdated);
  if (Preferences::instance()->isShowKeyframesOnXsheetCellAreaEnabled())
    drawKeyframe(p, toBeUpdated);
  drawNotes(p, toBeUpdated);

  // focus cell border
  drawFocusCellBorder(p);

  if (getDragTool()) getDragTool()->drawCellsArea(p);
}

//-----------------------------------------------------------------------------

class CycleUndo final : public TUndo {
  TStageObject *m_pegbar;
  CellArea *m_area;

public:
  // indices sono le colonne inserite
  CycleUndo(TStageObject *pegbar, CellArea *area)
      : m_pegbar(pegbar), m_area(area) {}
  void undo() const override {
    m_pegbar->enableCycle(!m_pegbar->isCycleEnabled());
    m_area->update();
    TApp::instance()->getCurrentScene()->setDirtyFlag(true);
    TApp::instance()->getCurrentObject()->notifyObjectIdChanged(false);
  }
  void redo() const override { undo(); }
  int getSize() const override { return sizeof *this; }

  QString getHistoryString() override {
    return QObject::tr("Toggle cycle of  %1")
        .arg(QString::fromStdString(m_pegbar->getName()));
  }
  int getHistoryType() override { return HistoryType::Xsheet; }
};
//----------------------------------------------------------
bool CellArea::isKeyFrameArea(int col, int row, QPoint mouseInCell) {
  if (!Preferences::instance()->isShowKeyframesOnXsheetCellAreaEnabled())
    return false;

  TXsheet *xsh = m_viewer->getXsheet();
  if (!xsh) return false;

  TStageObject *pegbar = xsh->getStageObject(m_viewer->getObjectId(col));
  int k0, k1;

  bool isKeyframeFrame = pegbar && pegbar->getKeyframeRange(k0, k1) &&
                         (k1 > k0 || k0 == row) && k0 <= row && row <= k1 + 1;

  if (!isKeyframeFrame) return false;

  const Orientation *o = m_viewer->orientation();
  QPoint frameAdj      = m_viewer->getFrameZoomAdjustment();

  if (o->isVerticalTimeline())
    return o->rect((col < 0) ? PredefinedRect::CAMERA_CELL
                             : PredefinedRect::KEYFRAME_AREA)
               .contains(mouseInCell) &&
           row < k1 + 1;

  QRect activeArea = (m_viewer->getFrameZoomFactor() >
                              o->dimension(PredefinedDimension::SCALE_THRESHOLD)
                          ? o->rect(PredefinedRect::KEYFRAME_AREA)
                          : o->rect(PredefinedRect::FRAME_MARKER_AREA));

  // If directly over keyframe icon, return true
  if (pegbar->isKeyframe(row) &&
      activeArea.translated(-frameAdj / 2).contains(mouseInCell) &&
      row < k1 + 1)
    return true;

  // In the white line area, if zoomed in.. narrow height by using frame marker
  // area since it has a narrower height
  if (m_viewer->getFrameZoomFactor() > 50)
    activeArea = o->rect(PredefinedRect::FRAME_MARKER_AREA);

  // Adjust left and/or right edge depending on which part of white line you are
  // on
  if (row > k0) activeArea.adjust(-activeArea.left(), 0, 0, 0);
  if (row < k1)
    activeArea.adjust(0, 0, (o->cellWidth() - activeArea.right()), 0);

  return activeArea.translated(-frameAdj / 2).contains(mouseInCell) &&
         row < k1 + 1;
}

void CellArea::mousePressEvent(QMouseEvent *event) {
  const Orientation *o = m_viewer->orientation();

  m_viewer->setQtModifiers(event->modifiers());
  assert(!m_isPanning);
  m_isMousePressed = true;
  QPoint frameAdj  = m_viewer->getFrameZoomAdjustment();
  if (event->button() == Qt::LeftButton) {
    assert(getDragTool() == 0);

    TPoint pos(event->pos().x(), event->pos().y());
    CellPosition cellPosition = m_viewer->xyToPosition(event->pos());
    int row                   = cellPosition.frame();
    int col                   = cellPosition.layer();
    QPoint cellTopLeft        = m_viewer->positionToXY(CellPosition(row, col));
    QPoint mouseInCell        = event->pos() - cellTopLeft;
    int x                     = mouseInCell.x();  // where in the cell click is

    // Check if a note is clicked
    TXshNoteSet *notes = m_viewer->getXsheet()->getNotes();
    int i;
    for (i = notes->getCount() - 1; i >= 0; i--) {
      int r       = notes->getNoteRow(i);
      int c       = notes->getNoteCol(i);
      QPoint xy   = m_viewer->positionToXY(CellPosition(r, c));
      TPointD pos = notes->getNotePos(i) + TPointD(xy.x(), xy.y());
      QRect rect  = o->rect(PredefinedRect::NOTE_ICON).translated(pos.x, pos.y);
      if (!rect.contains(event->pos())) continue;
      setDragTool(XsheetGUI::DragTool::makeNoteMoveTool(m_viewer));
      m_viewer->setCurrentNoteIndex(i);
      m_viewer->dragToolClick(event);
      event->accept();
      update();
      return;
    }
    // If I have not clicked on a note, and there's a note selected, deselect it
    if (m_viewer->getCurrentNoteIndex() >= 0) m_viewer->setCurrentNoteIndex(-1);

    TXsheet *xsh       = m_viewer->getXsheet();
    TXshColumn *column = xsh->getColumn(col);

    // Check if it's the sound column
    bool isSoundColumn = false;
    if (column) {
      TXshSoundColumn *soundColumn = column->getSoundColumn();
      isSoundColumn                = (!soundColumn) ? false : true;
    }

    // TObjectHandle *oh = TApp::instance()->getCurrentObject();
    // oh->setObjectId(m_viewer->getObjectId(col));

    // gmt, 28/12/2009. Non dovrebbe essere necessario, visto che dopo
    // verra cambiata la colonna e quindi l'oggetto corrente
    // Inoltre, facendolo qui, c'e' un problema con il calcolo del dpi
    // (cfr. SceneViewer::onLevelChanged()): setObjectId() chiama (in cascata)
    // onLevelChanged(), ma la colonna corrente risulta ancora quella di prima e
    // quindi
    // il dpi viene calcolato male. Quando si cambia la colonna l'oggetto
    // corrente risulta
    // gia' aggiornato e quindi non ci sono altre chiamate a onLevelChanged()
    // cfr bug#5235

    TStageObject *pegbar = xsh->getStageObject(m_viewer->getObjectId(col));

    if (Preferences::instance()->isShowKeyframesOnXsheetCellAreaEnabled()) {
      // only if key frame area is active
      int k0, k1;
      bool isKeyframeFrame = pegbar && pegbar->getKeyframeRange(k0, k1) &&
                             (k1 > k0 || k0 == row) && k0 <= row &&
                             row <= k1 + 1;
      bool accept = false;

      if (isKeyframeFrame &&
          isKeyFrameArea(col, row,
                         mouseInCell)) {  // They are in the keyframe selection
        if (pegbar->isKeyframe(row))      // in the keyframe
        {
          m_viewer->setCurrentRow(
              row);  // If you click on the key, change the current row as well
          setDragTool(XsheetGUI::DragTool::makeKeyframeMoverTool(m_viewer));
          accept = true;
        } else {
          int r0, r1;
          double e0, e1;
          int rh0, rh1;
          if (pegbar->getKeyframeSpan(row, r0, e0, r1, e1) &&
              getEaseHandles(r0, r1, e0, e1, rh0, rh1)) {
            if (rh0 == row) {  // in a keyframe handle
              setDragTool(XsheetGUI::DragTool::makeKeyFrameHandleMoverTool(
                  m_viewer, true, r0));
              accept = true;
            } else if (rh1 == row) {  // in a keyframe handle
              setDragTool(XsheetGUI::DragTool::makeKeyFrameHandleMoverTool(
                  m_viewer, false, r1));
              accept = true;
            }
          }
        }
      } else if (isKeyframeFrame && row == k1 + 1 &&
                 o->rect((col < 0) ? PredefinedRect::CAMERA_LOOP_ICON
                                   : PredefinedRect::LOOP_ICON)
                     .contains(mouseInCell)) {  // cycle toggle
        CycleUndo *undo = new CycleUndo(pegbar, this);
        undo->redo();
        TUndoManager::manager()->add(undo);
        accept = true;
      }
      if (accept) {
        m_viewer->dragToolClick(event);
        event->accept();
        update();
        return;
      }
      // keep searching
    }

    if (m_levelExtenderRect.contains(pos.x, pos.y)) {
      m_viewer->getKeyframeSelection()->selectNone();
      if (event->modifiers() & Qt::ControlModifier)
        setDragTool(
            XsheetGUI::DragTool::makeLevelExtenderTool(m_viewer, false));
      else
        setDragTool(XsheetGUI::DragTool::makeLevelExtenderTool(m_viewer, true));
    } else if (event->modifiers() & Qt::ControlModifier &&
               m_upperLevelExtenderRect.contains(pos.x, pos.y)) {
      m_viewer->getKeyframeSelection()->selectNone();
      setDragTool(
          XsheetGUI::DragTool::makeLevelExtenderTool(m_viewer, false, true));
    } else if ((!xsh->getCell(row, col).isEmpty()) &&
               o->rect(PredefinedRect::DRAG_AREA)
                   .adjusted(0, 0, -frameAdj.x(), -frameAdj.y())
                   .contains(mouseInCell)) {
      TXshColumn *column = xsh->getColumn(col);
      if (column && !m_viewer->getCellSelection()->isCellSelected(row, col)) {
        int r0, r1;
        column->getLevelRange(row, r0, r1);
        if (event->modifiers() & Qt::ControlModifier) {
          m_viewer->getCellKeyframeSelection()->makeCurrent();
          m_viewer->getCellKeyframeSelection()->selectCellsKeyframes(r0, col,
                                                                     r1, col);
        } else {
          m_viewer->getKeyframeSelection()->selectNone();
          m_viewer->getCellSelection()->makeCurrent();
          m_viewer->getCellSelection()->selectCells(r0, col, r1, col);
        }
        TApp::instance()->getCurrentSelection()->notifySelectionChanged();
      }
      TSelection *selection =
          TApp::instance()->getCurrentSelection()->getSelection();
      if (TCellKeyframeSelection *cellKeyframeSelection =
              dynamic_cast<TCellKeyframeSelection *>(selection))
        setDragTool(XsheetGUI::DragTool::makeCellKeyframeMoverTool(m_viewer));
      else if (Preferences::instance()->getDragCellsBehaviour() != 2)
        setDragTool(XsheetGUI::DragTool::makeLevelMoverTool(m_viewer));
    } else {
      m_viewer->getKeyframeSelection()->selectNone();
      if (isSoundColumn && o->rect(PredefinedRect::PREVIEW_TRACK)
                               .adjusted(0, 0, -frameAdj.x(), -frameAdj.y())
                               .contains(mouseInCell))
        setDragTool(XsheetGUI::DragTool::makeSoundScrubTool(
            m_viewer, column->getSoundColumn()));
      else if (isSoundColumn &&
               rectContainsPos(m_soundLevelModifyRects, event->pos()))
        setDragTool(XsheetGUI::DragTool::makeSoundLevelModifierTool(m_viewer));
      else
        setDragTool(XsheetGUI::DragTool::makeSelectionTool(m_viewer));
    }
    m_viewer->dragToolClick(event);
  } else if (event->button() == Qt::MidButton) {
    m_pos       = event->pos();
    m_isPanning = true;
  }
  event->accept();
  update();
}

//-----------------------------------------------------------------------------

void CellArea::mouseMoveEvent(QMouseEvent *event) {
  const Orientation *o = m_viewer->orientation();
  QPoint frameAdj      = m_viewer->getFrameZoomAdjustment();

  m_viewer->setQtModifiers(event->modifiers());
  setCursor(Qt::ArrowCursor);
  QPoint pos        = event->pos();
  QRect visibleRect = visibleRegion().boundingRect();
  if (m_isPanning) {
    // Pan tasto centrale
    m_viewer->scroll(m_pos - pos);
    return;
  }
  if ((event->buttons() & Qt::LeftButton) != 0 &&
      !visibleRegion().contains(pos)) {
    QRect bounds = visibleRegion().boundingRect();
    m_viewer->setAutoPanSpeed(bounds, pos);
  } else
    m_viewer->stopAutoPan();

  m_pos = pos;
  if (getDragTool()) {
    getDragTool()->onDrag(event);
    return;
  }

  CellPosition cellPosition = m_viewer->xyToPosition(pos);
  int row                   = cellPosition.frame();
  int col                   = cellPosition.layer();
  QPoint cellTopLeft        = m_viewer->positionToXY(CellPosition(row, col));
  int x                     = m_pos.x() - cellTopLeft.x();
  int y                     = m_pos.y() - cellTopLeft.y();
  QPoint mouseInCell        = m_pos - cellTopLeft;

  TXsheet *xsh = m_viewer->getXsheet();

  // Verifico se e' una colonna sound
  TXshColumn *column     = xsh->getColumn(col);
  bool isSoundColumn     = false;
  bool isZeraryColumn    = false;
  bool isSoundTextColumn = false;
  if (column) {
    TXshSoundColumn *soundColumn         = column->getSoundColumn();
    isSoundColumn                        = (!soundColumn) ? false : true;
    TXshZeraryFxColumn *zeraryColumn     = column->getZeraryFxColumn();
    isZeraryColumn                       = (!zeraryColumn) ? false : true;
    TXshSoundTextColumn *soundTextColumn = column->getSoundTextColumn();
    isSoundTextColumn                    = (!soundTextColumn) ? false : true;
  }

  TStageObject *pegbar = xsh->getStageObject(m_viewer->getObjectId(col));
  int k0, k1;
  bool isKeyframeFrame =
      Preferences::instance()->isShowKeyframesOnXsheetCellAreaEnabled() &&
      pegbar && pegbar->getKeyframeRange(k0, k1) && k0 <= row && row <= k1 + 1;

  if (isKeyframeFrame && isKeyFrameArea(col, row, mouseInCell)) {
    if (pegbar->isKeyframe(row))  // key frame
      m_tooltip = tr("Click to select keyframe, drag to move it");
    else {
      int r0, r1;
      double e0, e1;
      int rh0, rh1;
      if (pegbar->getKeyframeSpan(row, r0, e0, r1, e1) &&
          getEaseHandles(r0, r1, e0, e1, rh0,
                         rh1)) {  // triangles in the segment between key frames
        if (rh0 == row)
          m_tooltip = tr("Click and drag to set the acceleration range");
        else if (rh1 == row)
          m_tooltip = tr("Click and drag to set the deceleration range");
      }
    }
  } else if (isKeyframeFrame && row == k1 + 1 &&
             o->rect((col < 0) ? PredefinedRect::CAMERA_LOOP_ICON
                               : PredefinedRect::LOOP_ICON)
                 .contains(mouseInCell))  // cycle toggle of key frames
    m_tooltip = tr("Set the cycle of previous keyframes");
  else if ((!xsh->getCell(row, col).isEmpty()) &&
           o->rect(PredefinedRect::DRAG_AREA)
               .adjusted(0, 0, -frameAdj.x(), -frameAdj.y())
               .contains(mouseInCell))
    m_tooltip = tr("Click and drag to move the selection");
  else if (isZeraryColumn)
    m_tooltip = QString::fromStdWString(column->getZeraryFxColumn()
                                            ->getZeraryColumnFx()
                                            ->getZeraryFx()
                                            ->getName());
  else if ((!isSoundColumn && !xsh->getCell(row, col).isEmpty()) &&  // x > 6 &&
           x < (o->cellWidth() - frameAdj.x())) {
    TXshCell cell          = xsh->getCell(row, col);
    TFrameId fid           = cell.getFrameId();
    std::wstring levelName = cell.m_level->getName();

    // convert the last one digit of the frame number to alphabet
    // Ex.  12 -> 1B    21 -> 2A   30 -> 3
    if (isSoundTextColumn)
      m_tooltip = cell.getSoundTextLevel()->getFrameText(
          cell.m_frameId.getNumber() - 1);
    else if (Preferences::instance()->isShowFrameNumberWithLettersEnabled()) {
      m_tooltip =
          (fid.isEmptyFrame() || fid.isNoFrame())
              ? QString::fromStdWString(levelName)
              : QString::fromStdWString(levelName) + QString(" ") +
                    m_viewer->getFrameNumberWithLetters(fid.getNumber());
    } else {
      QString frameNumber("");
      if (fid.getNumber() >= 0) frameNumber = QString::number(fid.getNumber());
      if (!fid.getLetter().isEmpty()) frameNumber += fid.getLetter();
      m_tooltip =
          QString((frameNumber.isEmpty()) ? QString::fromStdWString(levelName)
                                          : QString::fromStdWString(levelName) +
                                                QString(" ") + frameNumber);
    }
  } else if (isSoundColumn && o->rect(PredefinedRect::PREVIEW_TRACK)
                                  .adjusted(0, 0, -frameAdj.x(), -frameAdj.y())
                                  .contains(mouseInCell))
    m_tooltip = tr("Click and drag to play");
  else if (m_levelExtenderRect.contains(pos))
    m_tooltip = tr("Click and drag to repeat selected cells");
  else if (isSoundColumn && rectContainsPos(m_soundLevelModifyRects, pos)) {
    if (o->isVerticalTimeline())
      setCursor(Qt::SplitVCursor);
    else
      setCursor(Qt::SplitHCursor);
    m_tooltip = tr("");
  } else
    m_tooltip = tr("");
}

//-----------------------------------------------------------------------------

void CellArea::mouseReleaseEvent(QMouseEvent *event) {
  m_viewer->setQtModifiers(Qt::KeyboardModifiers());
  m_isMousePressed = false;
  m_viewer->stopAutoPan();
  m_isPanning = false;
  m_viewer->dragToolRelease(event);
}

//-----------------------------------------------------------------------------

void CellArea::mouseDoubleClickEvent(QMouseEvent *event) {
  const Orientation *o = m_viewer->orientation();
  TPoint pos(event->pos().x(), event->pos().y());
  CellPosition cellPosition = m_viewer->xyToPosition(event->pos());
  int row                   = cellPosition.frame();
  int col                   = cellPosition.layer();
  // Se la colonna e' sound non devo fare nulla
  TXshColumn *column = m_viewer->getXsheet()->getColumn(col);
  if (column && column->getSoundColumn()) return;

  // Se ho cliccato su una nota devo aprire il popup
  TXshNoteSet *notes = m_viewer->getXsheet()->getNotes();
  int i;
  for (i = notes->getCount() - 1; i >= 0; i--) {
    int r       = notes->getNoteRow(i);
    int c       = notes->getNoteCol(i);
    QPoint xy   = m_viewer->positionToXY(CellPosition(r, c));
    TPointD pos = notes->getNotePos(i) + TPointD(xy.x(), xy.y());
    QRect rect  = o->rect(PredefinedRect::NOTE_ICON).translated(pos.x, pos.y);
    if (!rect.contains(event->pos())) continue;
    m_viewer->setCurrentNoteIndex(i);
    m_viewer->getNotesWidget().at(i)->openNotePopup();
    return;
  }

  TObjectHandle *oh = TApp::instance()->getCurrentObject();
  oh->setObjectId(m_viewer->getObjectId(col));

  if (col == -1) return;

  // in modalita' xsheet as animation sheet non deve essere possibile creare
  // livelli con doppio click: se la cella e' vuota non bisogna fare nulla
  if ((Preferences::instance()->isAnimationSheetEnabled() &&
       m_viewer->getXsheet()->getCell(row, col).isEmpty())) {
    TXshColumn *column = m_viewer->getXsheet()->getColumn(col);
    if (!column ||
        column->getColumnType() != TXshColumn::ColumnType::eSoundTextType)
      return;
  }

  int colCount = m_viewer->getCellSelection()->getSelectedCells().getColCount();

  m_renameCell->showInRowCol(row, col, colCount > 1);
}

//-----------------------------------------------------------------------------

void CellArea::registerContextMenus() {
  QMenu *noteMenu = new QMenu();
  QMap<QString, QString> menuCommandLabels;
  createNoteMenu(*noteMenu, menuCommandLabels);
  registerMenu(noteMenu, CMenuType_Note, menuCommandLabels);

  QMenu *keyMenu = new QMenu();
  createKeyMenu(*keyMenu);
  registerMenu(keyMenu, CMenuType_Key);

  QMenu *keyLineMenu = new QMenu();
  QMap<QString, QString> keyLineCommandLabels;
  QMap<CONDITION_MASKS, QString> keyLineConditionDescriptions;
  createKeyLineMenu(*keyLineMenu, keyLineCommandLabels,
                    keyLineConditionDescriptions);
  registerMenu(keyLineMenu, CMenuType_KeyLine, keyLineCommandLabels,
               keyLineConditionDescriptions);

  QMenu *cellMenu = new QMenu();
  QMap<QString, QString> cellCommandLabels;
  QMap<CONDITION_MASKS, QString> cellConditionDescriptions;
  createCellMenu(*cellMenu, cellCommandLabels, cellConditionDescriptions);
  registerMenu(cellMenu, CMenuType_Cell, cellCommandLabels,
               cellConditionDescriptions);
}

//-----------------------------------------------------------------------------

QAction *CellArea::customContextAction(const QString &cmdId) {
  QAction *action = nullptr;
  bool ret        = true;
  //--- Note area
  if (cmdId == CMenu_OpenMemo) {
    action = new QAction(tr("Open Memo"));
    ret = ret && connect(action, SIGNAL(triggered()), this, SLOT(openNote()));
  } else if (cmdId == CMenu_DeleteMemo) {
    action = new QAction(tr("Delete Memo"));
    ret = ret && connect(action, SIGNAL(triggered()), this, SLOT(deleteNote()));
  }
  //--- KeyLine area
  else if (cmdId == CMenu_Interp1s || cmdId == CMenu_Interp2s ||
           cmdId == CMenu_Interp3s || cmdId == CMenu_Interp4s) {
    int step = (cmdId == CMenu_Interp1s)   ? 1
               : (cmdId == CMenu_Interp2s) ? 2
               : (cmdId == CMenu_Interp3s) ? 3
                                           : 4;
    action =
        new QAction(tr("Interpolation on %1's").arg(QString::number(step)));
    QList<QVariant> data = m_contextMenuData.toList();
    data.push_front(step);
    action->setData(data);
    ret = ret &&
          connect(action, SIGNAL(triggered()), this, SLOT(onStepChanged()));
  }
  //--- Cell area
  else if (cmdId == CMenu_ReplaceLevelWith) {
    // replace with another level in scene cast
    std::vector<TXshLevel *> levels;
    TApp::instance()->getCurrentScene()->getScene()->getLevelSet()->listLevels(
        levels);
    QMenu *replaceMenu = new QMenu(tr("Replace with"));
    if (!levels.empty()) {
      ret = ret && connect(replaceMenu, SIGNAL(triggered(QAction *)), this,
                           SLOT(onReplaceByCastedLevel(QAction *)));
      for (int i = 0; i < (int)levels.size(); i++) {
        if (!levels[i]->getSimpleLevel() && !levels[i]->getChildLevel())
          continue;

        if (levels[i]->getChildLevel() &&
            !TApp::instance()->getCurrentXsheet()->getXsheet()->isLevelUsed(
                levels[i]))
          continue;

        QString tmpLevelName = QString::fromStdWString(levels[i]->getName());
        QAction *tmpAction   = new QAction(tmpLevelName, replaceMenu);
        tmpAction->setData(tmpLevelName);
        replaceMenu->addAction(tmpAction);
      }
    }
    action = replaceMenu->menuAction();
  } else if (cmdId == CMenu_CellMark) {
    QList<QVariant> data = m_contextMenuData.toList();
    assert(data.size() == 2);
    int row = data[0].toInt();
    int col = data[1].toInt();
    TXshCellColumn *cellColumn =
        m_viewer->getXsheet()->getColumn(col)->getCellColumn();
    assert(cellColumn);
    QMenu *marksMenu    = new QMenu(tr("Cell Mark"), this);
    int markId          = cellColumn->getCellMark(row);
    QAction *markAction = marksMenu->addAction(tr("None"));
    markAction->setCheckable(true);
    markAction->setChecked(markId == -1);
    markAction->setEnabled(markId != -1);
    markAction->setData(QList<QVariant>{row, col, -1});
    ret = ret &&
          connect(markAction, SIGNAL(triggered()), this, SLOT(onSetCellMark()));
    QList<TSceneProperties::CellMark> marks = TApp::instance()
                                                  ->getCurrentScene()
                                                  ->getScene()
                                                  ->getProperties()
                                                  ->getCellMarks();
    int curId = 0;
    for (auto mark : marks) {
      QString label = QString("%1: %2").arg(curId).arg(mark.name);
      markAction    = marksMenu->addAction(getColorChipIcon(mark.color), label);
      markAction->setCheckable(true);
      markAction->setChecked(markId == curId);
      markAction->setEnabled(markId != curId);
      markAction->setData(QList<QVariant>{row, col, curId});
      ret = ret && connect(markAction, SIGNAL(triggered()), this,
                           SLOT(onSetCellMark()));
      curId++;
    }
    action = marksMenu->menuAction();
  }

  assert(ret);
  assert(action);
  return action;
}
//-----------------------------------------------------------------------------

void CellArea::contextMenuEvent(QContextMenuEvent *event) {
  const Orientation *o = m_viewer->orientation();
  TPoint pos(event->pos().x(), event->pos().y());
  CellPosition cellPosition = m_viewer->xyToPosition(event->pos());
  int row                   = cellPosition.frame();
  int col                   = cellPosition.layer();
  TXshCell cell             = m_viewer->getXsheet()->getCell(row, col);

  QMenu menu(this);

  // Verifico se ho cliccato su una nota
  TXshNoteSet *notes = m_viewer->getXsheet()->getNotes();
  int i;
  for (i = notes->getCount() - 1; i >= 0; i--) {
    int r       = notes->getNoteRow(i);
    int c       = notes->getNoteCol(i);
    QPoint xy   = m_viewer->positionToXY(CellPosition(r, c));
    TPointD pos = notes->getNotePos(i) + TPointD(xy.x(), xy.y());
    QRect rect  = o->rect(PredefinedRect::NOTE_ICON).translated(pos.x, pos.y);
    if (!rect.contains(event->pos())) continue;
    m_viewer->setCurrentNoteIndex(i);
    getMenu(menu, CMenuType_Note);
    if (!menu.isEmpty()) menu.exec(event->globalPos());
    return;
  }

  TXsheet *xsh         = m_viewer->getXsheet();
  int x0               = m_viewer->positionToXY(cellPosition).x();
  int x                = pos.x - x0;
  TStageObject *pegbar = xsh->getStageObject(m_viewer->getObjectId(col));
  int k0, k1;
  int r0, r1, c0, c1;
  m_viewer->getCellSelection()->getSelectedCells(r0, c0, r1, c1);

  QPoint cellTopLeft = m_viewer->positionToXY(CellPosition(row, col));
  QPoint mouseInCell = event->pos() - cellTopLeft;
  bool isKeyframeFrame =
      Preferences::instance()->isShowKeyframesOnXsheetCellAreaEnabled() &&
      pegbar && pegbar->getKeyframeRange(k0, k1) && k0 <= row && row <= k1 + 1;

  if (isKeyframeFrame && isKeyFrameArea(col, row, mouseInCell)) {
    TStageObjectId objectId;
    if (col < 0)
      objectId = TStageObjectId::CameraId(xsh->getCameraColumnIndex());
    else {  // Set the current column and the current object
      objectId = TStageObjectId::ColumnId(col);
      m_viewer->setCurrentColumn(col);
    }
    TApp::instance()->getCurrentObject()->setObjectId(objectId);
    m_viewer->setCurrentRow(row);
    if (pegbar->isKeyframe(row)) {  // clicking on keyframes
      TKeyframeSelection *keyframeSelection = m_viewer->getKeyframeSelection();
      keyframeSelection->select(row, col);
      keyframeSelection->makeCurrent();
      getMenu(menu, CMenuType_Key);
    } else if (!xsh->getColumn(col) ||
               !xsh->getColumn(col)
                    ->isLocked())  // on the line between two keyframes
      retrieveKeyLineMenu(menu, row, col);
  } else if (m_viewer->getCellSelection()->isCellSelected(
                 row, col) &&  // La cella e' selezionata
             (abs(r1 - r0) > 0 ||
              abs(c1 - c0) >
                  0))  // Il numero di celle selezionate e' maggiore di 1
  {                    // Sono su una selezione di celle
    m_viewer->setCurrentColumn(col);
    int e, f;
    bool areCellsOccupied = false;
    for (e = r0; e <= r1; e++) {
      for (f = c0; f <= c1; f++)
        if (!xsh->getCell(e, f).isEmpty()) {
          areCellsOccupied = true;
          break;
        }
      if (areCellsOccupied) break;
    }
    retrieveCellMenu(menu, areCellsOccupied, cell, row, col);
  } else {
    m_viewer->getCellSelection()->makeCurrent();
    m_viewer->getCellSelection()->selectCell(row, col);
    m_viewer->setCurrentColumn(col);

    retrieveCellMenu(menu, !cell.isEmpty(), cell, row, col);
  }

  if (!menu.isEmpty()) menu.exec(event->globalPos());
}

//-----------------------------------------------------------------------------

void CellArea::dragEnterEvent(QDragEnterEvent *e) {
  bool isResourceOrFolderDrop =
      acceptResourceOrFolderDrop(e->mimeData()->urls());
  if (isResourceOrFolderDrop ||
      e->mimeData()->hasFormat(CastItems::getMimeFormat()) ||
      e->mimeData()->hasFormat("application/vnd.toonz.drawings")) {
    setDragTool(XsheetGUI::DragTool::makeDragAndDropDataTool(m_viewer));
    // For file dragging force CopyAction
    if (isResourceOrFolderDrop) e->setDropAction(Qt::CopyAction);
    m_viewer->dragToolClick(e);
    // For files, don't accept original proposed action in case it's a move
    isResourceOrFolderDrop ? e->accept() : e->acceptProposedAction();
  }
}

//-----------------------------------------------------------------------------

void CellArea::dragLeaveEvent(QDragLeaveEvent *e) {
  if (!getDragTool()) return;
  m_viewer->dragToolLeave(e);
}

//-----------------------------------------------------------------------------

void CellArea::dragMoveEvent(QDragMoveEvent *e) {
  if (!getDragTool()) return;
  bool isResourceOrFolderDrop =
      acceptResourceOrFolderDrop(e->mimeData()->urls());
  // For file dragging force CopyAction
  if (isResourceOrFolderDrop) e->setDropAction(Qt::CopyAction);
  m_viewer->dragToolDrag(e);
  // For files, don't accept original proposed action in case it's a move
  isResourceOrFolderDrop ? e->accept() : e->acceptProposedAction();
}

//-----------------------------------------------------------------------------

void CellArea::dropEvent(QDropEvent *e) {
  if (!getDragTool()) return;
  m_viewer->dragToolRelease(e);
  if (e->source() == this) {
    e->setDropAction(Qt::MoveAction);
    e->accept();
  } else if (acceptResourceOrFolderDrop(e->mimeData()->urls())) {
    // For file dragging force CopyAction
    e->setDropAction(Qt::CopyAction);
    // For files, don't accept original proposed action in case it's a move
    e->accept();
  } else
    e->acceptProposedAction();
}

//-----------------------------------------------------------------------------

bool CellArea::event(QEvent *event) {
  QEvent::Type type = event->type();
  if (type == QEvent::ToolTip) {
    if (!m_tooltip.isEmpty())
      QToolTip::showText(mapToGlobal(m_pos), m_tooltip);
    else
      QToolTip::hideText();
  }
  if (type == QEvent::WindowDeactivate && m_isMousePressed) {
    QMouseEvent e(QEvent::MouseButtonRelease, m_pos, Qt::LeftButton,
                  Qt::NoButton, Qt::NoModifier);
    mouseReleaseEvent(&e);
  }
  return QWidget::event(event);
}
//-----------------------------------------------------------------------------

void CellArea::onControlPressed(bool pressed) {
  isCtrlPressed = pressed;
  update();
}

//-----------------------------------------------------------------------------

const bool CellArea::isControlPressed() { return isCtrlPressed; }

//-----------------------------------------------------------------------------
void CellArea::createCellMenu(
    QMenu &menu, QMap<QString, QString> &commandLabels,
    QMap<CONDITION_MASKS, QString> &conditionDescriptions) {
  conditionDescriptions = {
      {Condition01, tr("At least one of the selected cells is not empty.",
                       "context menu condition")},
      {Condition02,
       tr("All of the selected cells are empty.", "context menu condition")},
      {Condition03,
       tr("At least one of the selected cells is not a sound level cell.",
          "context menu condition")},
      {Condition04,
       tr("At least one of the selected cells is not a note level cell.",
          "context menu condition")},
      {Condition05, tr("All of the selected cells are in the camera column.",
                       "context menu condition")},
      {Condition06, tr("The selection contains any type of image level cell.",
                       "context menu condition")},
      {Condition07, tr("The selection contains a raster level cell.",
                       "context menu condition")},
      {Condition08, tr("NOT Condition7 AND the selection contains a toonz "
                       "raster level cell.",
                       "context menu condition")},
      {Condition09, tr("Preferences > Xsheet > Paste Cells Behavior is set to "
                       "\"Insert Paste Whole Data\"",
                       "context menu condition")},
      {Condition10, tr("Preferences > Xsheet > Paste Cells Behavior is set to "
                       "\"Overwrite Paste Cell Numbers\"",
                       "context menu condition")},
      {Condition11, tr("The current level is any type of image level OR sound "
                       "level cells are selected",
                       "context menu condition")},
      {Condition12, tr("The current level is any type of image level.",
                       "context menu condition")},
      {Condition13,
       tr("A sub-xsheet level cell is clicked.", "context menu condition")},
      {Condition14,
       tr("The current level is any type of image level OR a sub-xsheet level.",
          "context menu condition")},
      {Condition15,
       tr("A zerary Fx cell is clicked.", "context menu condition")},
      {Condition16, tr("Condition3 OR condition14.", "context menu condition")},
      {Condition17, tr("Condition4 OR condition15.", "context menu condition")},
      {Condition18,
       tr("Any column with cells is clicked.", "context menu condition")},
  };

  auto createMenu = [&](const char *title) {
    QMenu *ret = new QMenu(tr(title), this);
    // store untranslated text to the icon text
    ret->menuAction()->setIconText(title);
    return ret;
  };

  //--- Condition03 :  !soundCellsSelected
  menu.addAction(MI_LoadLevel)->setData(Condition03);
  menu.addAction(MI_NewLevel)->setData(Condition03);
  menu.addSeparator()->setData(Condition03);

  //--- Condition01 : isCellSelected
  {
    //--- Condition15 : cell.m_level && cell.m_level->getZeraryFxLevel()
    menu.addAction(MI_FxParamEditor)->setData(Condition01 | Condition15);
    //--- Condition04 : !soundTextCellsSelected
    menu.addAction(MI_LevelSettings)->setData(Condition01 | Condition04);
    //--- Condition17 : addSeparator
    menu.addSeparator()->setData(Condition01 | Condition17);

    //--- Condition03 : !soundCellsSelected
    {
      QMenu *reframeSubMenu = createMenu(QT_TR_NOOP("Reframe"));
      {
        reframeSubMenu->addAction(MI_Reframe1)
            ->setData(Condition01 | Condition03);
        reframeSubMenu->addAction(MI_Reframe2)
            ->setData(Condition01 | Condition03);
        reframeSubMenu->addAction(MI_Reframe3)
            ->setData(Condition01 | Condition03);
        reframeSubMenu->addAction(MI_Reframe4)
            ->setData(Condition01 | Condition03);
        reframeSubMenu->addAction(MI_ReframeWithEmptyInbetweens)
            ->setData(Condition01 | Condition03);
      }
      menu.addMenu(reframeSubMenu)->setData(Condition01 | Condition03);

      QMenu *stepSubMenu = createMenu(QT_TR_NOOP("Step"));
      {
        stepSubMenu->addAction(MI_Step2)->setData(Condition01 | Condition03);
        stepSubMenu->addAction(MI_Step3)->setData(Condition01 | Condition03);
        stepSubMenu->addAction(MI_Step4)->setData(Condition01 | Condition03);
        stepSubMenu->addAction(MI_ResetStep)
            ->setData(Condition01 | Condition03);
        stepSubMenu->addAction(MI_IncreaseStep)
            ->setData(Condition01 | Condition03);
        stepSubMenu->addAction(MI_DecreaseStep)
            ->setData(Condition01 | Condition03);
      }
      menu.addMenu(stepSubMenu)->setData(Condition01 | Condition03);

      QMenu *eachSubMenu = createMenu(QT_TR_NOOP("Each"));
      {
        eachSubMenu->addAction(MI_Each2)->setData(Condition01 | Condition03);
        eachSubMenu->addAction(MI_Each3)->setData(Condition01 | Condition03);
        eachSubMenu->addAction(MI_Each4)->setData(Condition01 | Condition03);
      }
      menu.addMenu(eachSubMenu)->setData(Condition01 | Condition03);

      //--- Condition04 :!soundTextCellsSelected
      {
        QMenu *editCellNumbersMenu =
            createMenu(QT_TR_NOOP("Edit Cell Numbers"));
        {
          editCellNumbersMenu->addAction(MI_Reverse)
              ->setData(Condition01 | Condition03 | Condition04);
          editCellNumbersMenu->addAction(MI_Swing)->setData(
              Condition01 | Condition03 | Condition04);
          editCellNumbersMenu->addAction(MI_Random)->setData(
              Condition01 | Condition03 | Condition04);
          editCellNumbersMenu->addAction(MI_Dup)->setData(
              Condition01 | Condition03 | Condition04);
          editCellNumbersMenu->addAction(MI_Rollup)->setData(
              Condition01 | Condition03 | Condition04);
          editCellNumbersMenu->addAction(MI_Rolldown)
              ->setData(Condition01 | Condition03 | Condition04);
          editCellNumbersMenu->addAction(MI_TimeStretch)
              ->setData(Condition01 | Condition03 | Condition04);
          editCellNumbersMenu->addAction(MI_AutoInputCellNumber)
              ->setData(Condition01 | Condition03 | Condition04);
        }
        menu.addMenu(editCellNumbersMenu)
            ->setData(Condition01 | Condition03 | Condition04);
      }
      menu.addAction(MI_FillEmptyCell)->setData(Condition01 | Condition03);

      menu.addSeparator()->setData(Condition01 | Condition03);

      //--- Condition04 :!soundTextCellsSelected
      menu.addAction(MI_Autorenumber)
          ->setData(Condition01 | Condition03 | Condition04);
    }

    //--- Condition04 :!soundTextCellsSelected
    {
      QMenu *replaceLevelMenu = createMenu(QT_TR_NOOP("Replace Level"));
      menu.addMenu(replaceLevelMenu)->setData(Condition01 | Condition04);

      replaceLevelMenu->addAction(MI_ReplaceLevel)
          ->setData(Condition01 | Condition04);
      replaceLevelMenu->addAction(MI_ReplaceParentDirectory)
          ->setData(Condition01 | Condition04);

      commandLabels.insert(CMenu_ReplaceLevelWith, tr("Replace with"));
      replaceLevelMenu->addAction(CMenu_ReplaceLevelWith)
          ->setData(Condition01 | Condition04);

      //--- Condition03 : !soundCellsSelected
      {
        //--- Condition08 : selectionContainTlvImage
        replaceLevelMenu->addAction(MI_RevertToCleanedUp)
            ->setData(Condition01 | Condition03 | Condition04 | Condition08);
        //--- Condition06 : selectionContainLevelImage
        replaceLevelMenu->addAction(MI_RevertToLastSaved)
            ->setData(Condition01 | Condition03 | Condition04 | Condition06);
        menu.addAction(MI_SetKeyframes)
            ->setData(Condition01 | Condition03 | Condition04);
        menu.addAction(MI_ShiftKeyframesDown)
            ->setData(Condition01 | Condition03 | Condition04);
        menu.addAction(MI_ShiftKeyframesUp)
            ->setData(Condition01 | Condition03 | Condition04);
      }
      menu.addSeparator()->setData(Condition01 | Condition04);
    }

    menu.addAction(MI_Cut)->setData(Condition01);
    menu.addAction(MI_Copy)->setData(Condition01);
    menu.addAction(MI_Paste)->setData(Condition01);

    QMenu *pasteSpecialMenu = createMenu(QT_TR_NOOP("Paste Special"));
    {
      pasteSpecialMenu->addAction(MI_PasteInto)->setData(Condition01);
      // the "standard" paste behavior for MI_Paste is specified in the
      // preferences. here we display the alternative behavior.
      //--- Condition09 : Preferences::instance()->getPasteCellsBehavior() == 0
      pasteSpecialMenu->addAction(MI_PasteNumbers)
          ->setData(Condition01 | Condition09);
      //--- Condition10 : Preferences::instance()->getPasteCellsBehavior() != 0
      pasteSpecialMenu->addAction(MI_PasteCellContent)
          ->setData(Condition01 | Condition10);
      //--- Condition04 :!soundTextCellsSelected
      pasteSpecialMenu->addAction(MI_PasteDuplicate)
          ->setData(Condition01 | Condition04);
    }
    menu.addMenu(pasteSpecialMenu)->setData(Condition01);

    menu.addAction(MI_Clear)->setData(Condition01);
    menu.addAction(MI_Insert)->setData(Condition01);
    //--- Condition04 :!soundTextCellsSelected
    {
      menu.addAction(MI_CreateBlankDrawing)->setData(Condition01 | Condition04);
      menu.addAction(MI_Duplicate)->setData(Condition01 | Condition04);
    }
    menu.addSeparator()->setData(Condition01);

    TXshSimpleLevel *sl = TApp::instance()->getCurrentLevel()->getSimpleLevel();
    //--- Condition11 :  (sl || soundCellsSelected)
    menu.addAction(MI_FileInfo)->setData(Condition01 | Condition11);
    //--- Condition12 :  (sl && (sl->getType() & LEVELCOLUMN_XSHLEVEL))
    menu.addAction(MI_ViewFile)->setData(Condition01 | Condition12);

    menu.addSeparator()->setData(Condition01);

    //--- Condition13 :  (!cell.isEmpty() && cell.m_level &&
    // cell.m_level->getChildLevel())
    {
      menu.addAction(MI_OpenChild)->setData(Condition01 | Condition13);
      menu.addSeparator()->setData(Condition01 | Condition13);
    }

    //--- Condition07 : selectionContainRasterImage
    {
      QMenu *editImageMenu = createMenu(QT_TR_NOOP("Edit Image"));
      {
        editImageMenu->addAction(MI_AdjustLevels)
            ->setData(Condition01 | Condition07);
        editImageMenu->addAction(MI_LinesFade)
            ->setData(Condition01 | Condition07);
        editImageMenu->addAction(MI_BrightnessAndContrast)
            ->setData(Condition01 | Condition07);
        editImageMenu->addAction(MI_Antialias)
            ->setData(Condition01 | Condition07);
        editImageMenu->addAction(MI_CanvasSize)
            ->setData(Condition01 | Condition07);
      }
      menu.addMenu(editImageMenu)->setData(Condition01 | Condition07);
    }

    //--- Condition08 : selectionContainTlvImage
    menu.addAction(MI_CanvasSize)->setData(Condition01 | Condition08);

    QMenu *lipSyncMenu = createMenu(QT_TR_NOOP("Lip Sync"));
    {
      //--- Condition14 :(sl || getChildLevel())
      {
        lipSyncMenu->addAction(MI_LipSyncPopup)
            ->setData(Condition01 | Condition14);
        lipSyncMenu->addAction(MI_AutoLipSyncPopup)
            ->setData(Condition01 | Condition14);
      }
      //--- Condition03 : !soundCellsSelected
      lipSyncMenu->addAction(MI_ImportMagpieFile)
          ->setData(Condition01 | Condition03);
    }
    //--- Condition16 : Condition03 || Condition14
    menu.addMenu(lipSyncMenu)->setData(Condition01 | Condition16);
  }
  //--- Condition02 : !isCellSelected (!Condition01)
  {
    menu.addAction(MI_CreateBlankDrawing)->setData(Condition02);
    menu.addSeparator()->setData(Condition02);
    menu.addAction(MI_FillEmptyCell)->setData(Condition02);
    //--- Condition05 : cameraCellsSelected
    {
      menu.addSeparator()->setData(Condition02 | Condition05);
      menu.addAction(MI_SetKeyframes)->setData(Condition02 | Condition05);
      menu.addAction(MI_ShiftKeyframesDown)->setData(Condition02 | Condition05);
      menu.addAction(MI_ShiftKeyframesUp)->setData(Condition02 | Condition05);
    }
  }

  // cell mark menu
  //--- Condition18 : cellColumn
  {
    commandLabels.insert(CMenu_CellMark, tr("Cell Mark"));
    menu.addAction(CMenu_CellMark)->setData(Condition18);
  }
}

//-----------------------------------------------------------------------------
void CellArea::retrieveCellMenu(QMenu &menu, bool isCellSelected, TXshCell cell,
                                int row, int col) {
  uint mask = 0;

  if (isCellSelected)
    mask |= Condition01;
  else
    mask |= Condition02;

  //--- selection conditions
  bool soundCellsSelected     = m_viewer->areSoundCellsSelected();
  bool soundTextCellsSelected = m_viewer->areSoundTextCellsSelected();
  bool cameraCellsSelected    = m_viewer->areCameraCellsSelected();

  if (!soundCellsSelected) mask |= Condition03;
  if (!soundTextCellsSelected) mask |= Condition04;
  if (cameraCellsSelected) mask |= Condition05;

  if (selectionContainLevelImage(m_viewer->getCellSelection(),
                                 m_viewer->getXsheet()))
    mask |= Condition06;
  if (selectionContainRasterImage(m_viewer->getCellSelection(),
                                  m_viewer->getXsheet()))
    mask |= Condition07;
  else if (selectionContainTlvImage(m_viewer->getCellSelection(),
                                    m_viewer->getXsheet()))
    mask |= Condition08;

  //--- preferences conditions
  if (Preferences::instance()->getPasteCellsBehavior() == 0)
    mask |= Condition09;
  else
    mask |= Condition10;

  //--- current level type conditions
  TXshSimpleLevel *sl = TApp::instance()->getCurrentLevel()->getSimpleLevel();
  if (sl || soundCellsSelected) mask |= Condition11;
  if (sl && (sl->getType() & LEVELCOLUMN_XSHLEVEL)) mask |= Condition12;
  if (!cell.isEmpty() && cell.m_level && cell.m_level->getChildLevel())
    mask |= Condition13;
  if (sl || (TApp::instance()->getCurrentLevel()->getLevel() &&
             TApp::instance()->getCurrentLevel()->getLevel()->getChildLevel()))
    mask |= Condition14;
  // open fx settings instead of level settings when clicked on zerary fx
  // level
  if (cell.m_level && cell.m_level->getZeraryFxLevel()) mask |= Condition15;

  // separator conditions
  if (mask & (Condition03 | Condition14)) {
    mask |= Condition16;
  }
  if (mask & (Condition04 | Condition15)) {
    mask |= Condition17;
  }

  // cell mark menu
  TXshCellColumn *cellColumn =
      (m_viewer->getXsheet()->getColumn(col))
          ? m_viewer->getXsheet()->getColumn(col)->getCellColumn()
          : nullptr;
  if (cellColumn) {
    mask |= Condition18;
  }

  // used in the cellmark commands
  m_contextMenuData = QList<QVariant>{row, col};

  getMenu(menu, CMenuType_Cell, mask);
}

//-----------------------------------------------------------------------------
/*! replace level with another level in the cast
 */
void CellArea::onReplaceByCastedLevel(QAction *action) {
  std::wstring levelName = action->data().toString().toStdWString();
  TXshLevel *level =
      TApp::instance()->getCurrentScene()->getScene()->getLevelSet()->getLevel(
          levelName);

  if (!level) return;
  TCellSelection *cellSelection = m_viewer->getCellSelection();
  if (cellSelection->isEmpty()) return;
  int r0, c0, r1, c1;
  cellSelection->getSelectedCells(r0, c0, r1, c1);

  bool changed = false;

  TUndoManager *um = TUndoManager::manager();
  um->beginBlock();

  TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
  for (int c = c0; c <= c1; c++) {
    for (int r = r0; r <= r1; r++) {
      TXshCell cell = xsh->getCell(r, c);
      if (!cell.m_level.getPointer() || cell.m_level.getPointer() == level)
        continue;

      TXshCell oldCell = cell;

      cell.m_level = TXshLevelP(level);
      xsh->setCell(r, c, cell);

      RenameCellUndo *undo = new RenameCellUndo(r, c, oldCell, cell);
      um->add(undo);

      changed = true;
    }
  }

  if (changed) TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();

  um->endBlock();
}

//-----------------------------------------------------------------------------

void CellArea::createKeyMenu(QMenu &menu) {
  menu.addAction(MI_SelectRowKeyframes);
  menu.addAction(MI_SelectColumnKeyframes);
  menu.addAction(MI_SelectAllKeyframes);
  menu.addAction(MI_SelectAllKeyframesNotBefore);
  menu.addAction(MI_SelectAllKeyframesNotAfter);
  menu.addAction(MI_SelectPreviousKeysInColumn);
  menu.addAction(MI_SelectFollowingKeysInColumn);
  menu.addAction(MI_SelectPreviousKeysInRow);
  menu.addAction(MI_SelectFollowingKeysInRow);
  menu.addAction(MI_InvertKeyframeSelection);
  menu.addSeparator();
  menu.addAction(MI_Cut);
  menu.addAction(MI_Copy);
  menu.addAction(MI_Paste);
  menu.addAction(MI_Clear);
  menu.addSeparator();
  menu.addAction(MI_ShiftKeyframesDown);
  menu.addAction(MI_ShiftKeyframesUp);
  menu.addSeparator();
  menu.addAction(MI_OpenFunctionEditor);
}

//-----------------------------------------------------------------------------

void CellArea::createKeyLineMenu(
    QMenu &menu, QMap<QString, QString> &commandLabels,
    QMap<CONDITION_MASKS, QString> &conditionDescriptions) {
  conditionDescriptions.insert(
      Condition01, tr("Speed handles are displayed on the clicked segment.",
                      "context menu condition"));
  menu.addAction(MI_SetAcceleration)->setData(Condition01);
  menu.addAction(MI_SetDeceleration)->setData(Condition01);
  menu.addAction(MI_SetConstantSpeed)->setData(Condition01);
  menu.addSeparator()->setData(Condition01);

  //----
  // Hmm I don't understand what this condition / command is for..
  conditionDescriptions.insert(
      Condition02, tr("NOT Condition1 AND the clicked segment is between "
                      "global key frames with interpolation other than linear.",
                      "context menu condition"));
  menu.addAction(MI_ResetInterpolation)->setData(Condition02);
  menu.addSeparator()->setData(Condition02);

  //----
  conditionDescriptions.insert(
      Condition03,
      tr("The interpolation type of clicked segment is not Constant.",
         "context menu condition"));
  menu.addAction(MI_UseConstantInterpolation)->setData(Condition03);
  conditionDescriptions.insert(
      Condition04,
      tr("The interpolation type of clicked segment is not Linear.",
         "context menu condition"));
  menu.addAction(MI_UseLinearInterpolation)->setData(Condition04);
  conditionDescriptions.insert(
      Condition05,
      tr("The interpolation type of clicked segment is not Speed In/Out.",
         "context menu condition"));
  menu.addAction(MI_UseSpeedInOutInterpolation)->setData(Condition05);
  conditionDescriptions.insert(
      Condition06,
      tr("The interpolation type of clicked segment is not Ease In/Out.",
         "context menu condition"));
  menu.addAction(MI_UseEaseInOutInterpolation)->setData(Condition06);
  conditionDescriptions.insert(
      Condition07,
      tr("The interpolation type of clicked segment is not Ease In/Out(%).",
         "context menu condition"));
  menu.addAction(MI_UseEaseInOutPctInterpolation)->setData(Condition07);
  conditionDescriptions.insert(
      Condition08,
      tr("The interpolation type of clicked segment is not Exponential.",
         "context menu condition"));
  menu.addAction(MI_UseExponentialInterpolation)->setData(Condition08);

  //----
  conditionDescriptions.insert(Condition09, tr("The camera column is clicked.",
                                               "context menu condition"));
  menu.addSeparator()->setData(Condition09);
  menu.addAction(MI_SetKeyframes)->setData(Condition09);
  menu.addAction(MI_ShiftKeyframesDown)->setData(Condition09);
  menu.addAction(MI_ShiftKeyframesUp)->setData(Condition09);

  menu.addSeparator();
  menu.addAction(CMenu_Interp1s);
  menu.addAction(CMenu_Interp2s);
  menu.addAction(CMenu_Interp3s);
  menu.addAction(CMenu_Interp4s);
  commandLabels.insert(CMenu_Interp1s, tr("Interpolation on 1's"));
  commandLabels.insert(CMenu_Interp2s, tr("Interpolation on 2's"));
  commandLabels.insert(CMenu_Interp3s, tr("Interpolation on 3's"));
  commandLabels.insert(CMenu_Interp4s, tr("Interpolation on 4's"));
}

//-----------------------------------------------------------------------------
void CellArea::retrieveKeyLineMenu(QMenu &menu, int row, int col) {
  uint mask = 0;

  TStageObject *pegbar =
      m_viewer->getXsheet()->getStageObject(m_viewer->getObjectId(col));
  CommandManager *cmdManager = CommandManager::instance();
  int r0, r1, rh0, rh1;
  double e0, e1;
  if (pegbar->getKeyframeSpan(row, r0, e0, r1, e1) &&
      getEaseHandles(r0, r1, e0, e1, rh0, rh1)) {
    mask |= Condition01;
  } else {
    // Se le due chiavi non sono linear aggiungo il comando ResetInterpolation
    // bool isR0FullK = pegbar->isFullKeyframe(r0);
    // bool isR1FullK = pegbar->isFullKeyframe(r1);
    // TDoubleKeyframe::Type r0Type =
    //    pegbar->getParam(TStageObject::T_X)->getKeyframeAt(r0).m_type;
    // TDoubleKeyframe::Type r1Type =
    //    pegbar->getParam(TStageObject::T_X)->getKeyframeAt(r1).m_prevType;
    if (isGlobalKeyFrameWithSameTypeDiffFromLinear(pegbar, r0) &&
        isGlobalKeyFrameWithSamePrevTypeDiffFromLinear(pegbar, r1)) {
      mask |= Condition02;
    }
  }

  TDoubleKeyframe::Type rType =
      pegbar->getParam(TStageObject::T_X)->getKeyframeAt(r0).m_type;

  if (rType != TDoubleKeyframe::Constant) mask |= Condition03;
  if (rType != TDoubleKeyframe::Linear) mask |= Condition04;
  if (rType != TDoubleKeyframe::SpeedInOut) mask |= Condition05;
  if (rType != TDoubleKeyframe::EaseInOut) mask |= Condition06;
  if (rType != TDoubleKeyframe::EaseInOutPercentage) mask |= Condition07;
  if (rType != TDoubleKeyframe::Exponential) mask |= Condition08;

  if (col < 0) {
    mask |= Condition09;
  }
  // used in the commands for changing interpolation
  m_contextMenuData = QList<QVariant>{r0, col};

  getMenu(menu, CMenuType_KeyLine, mask);
}

//-----------------------------------------------------------------------------

void CellArea::createNoteMenu(QMenu &menu,
                              QMap<QString, QString> &commandLabels) {
  menu.addAction(CMenu_OpenMemo);
  menu.addAction(CMenu_DeleteMemo);
  commandLabels.insert(CMenu_OpenMemo, tr("Open Memo"));
  commandLabels.insert(CMenu_DeleteMemo, tr("Delete Memo"));
}

//-----------------------------------------------------------------------------

void CellArea::openNote() {
  TXshNoteSet *notes = m_viewer->getXsheet()->getNotes();
  int currentIndex   = m_viewer->getCurrentNoteIndex();
  m_viewer->getNotesWidget().at(currentIndex)->openNotePopup();
}

//-----------------------------------------------------------------------------

void CellArea::deleteNote() {
  TXshNoteSet *notes = m_viewer->getXsheet()->getNotes();
  int currentIndex   = m_viewer->getCurrentNoteIndex();
  notes->removeNote(currentIndex);
  m_viewer->discardNoteWidget();
}

//-----------------------------------------------------------------------------

void CellArea::onStepChanged() {
  QAction *act         = qobject_cast<QAction *>(sender());
  QList<QVariant> list = act->data().toList();
  int step             = list.at(0).toInt();
  int frame            = list.at(1).toInt();
  int col              = list.at(2).toInt();

  TUndoManager::manager()->beginBlock();
  TStageObject *stageObject =
      m_viewer->getXsheet()->getStageObject(m_viewer->getObjectId(col));
  TDoubleParam *param = stageObject->getParam(TStageObject::T_Angle);
  int keyFrameIndex   = param->getClosestKeyframe(frame);
  if (keyFrameIndex >= 0) {
    setParamStep(keyFrameIndex, step,
                 stageObject->getParam(TStageObject::T_Angle));
  }

  param         = stageObject->getParam(TStageObject::T_X);
  keyFrameIndex = param->getClosestKeyframe(frame);
  if (keyFrameIndex >= 0) {
    setParamStep(keyFrameIndex, step, stageObject->getParam(TStageObject::T_X));
  }
  param         = stageObject->getParam(TStageObject::T_Y);
  keyFrameIndex = param->getClosestKeyframe(frame);
  if (keyFrameIndex >= 0) {
    setParamStep(keyFrameIndex, step, stageObject->getParam(TStageObject::T_Y));
  }
  param         = stageObject->getParam(TStageObject::T_Z);
  keyFrameIndex = param->getClosestKeyframe(frame);
  if (keyFrameIndex >= 0) {
    setParamStep(keyFrameIndex, step, stageObject->getParam(TStageObject::T_Z));
  }
  param         = stageObject->getParam(TStageObject::T_SO);
  keyFrameIndex = param->getClosestKeyframe(frame);
  if (keyFrameIndex >= 0) {
    setParamStep(keyFrameIndex, step,
                 stageObject->getParam(TStageObject::T_SO));
  }
  param         = stageObject->getParam(TStageObject::T_ScaleX);
  keyFrameIndex = param->getClosestKeyframe(frame);
  if (keyFrameIndex >= 0) {
    setParamStep(keyFrameIndex, step,
                 stageObject->getParam(TStageObject::T_ScaleX));
  }
  param         = stageObject->getParam(TStageObject::T_ScaleY);
  keyFrameIndex = param->getClosestKeyframe(frame);
  if (keyFrameIndex >= 0) {
    setParamStep(keyFrameIndex, step,
                 stageObject->getParam(TStageObject::T_ScaleY));
  }
  param         = stageObject->getParam(TStageObject::T_Scale);
  keyFrameIndex = param->getClosestKeyframe(frame);
  if (keyFrameIndex >= 0) {
    setParamStep(keyFrameIndex, step,
                 stageObject->getParam(TStageObject::T_Scale));
  }
  param         = stageObject->getParam(TStageObject::T_Path);
  keyFrameIndex = param->getClosestKeyframe(frame);
  if (keyFrameIndex >= 0) {
    setParamStep(keyFrameIndex, step,
                 stageObject->getParam(TStageObject::T_Path));
  }
  param         = stageObject->getParam(TStageObject::T_ShearX);
  keyFrameIndex = param->getClosestKeyframe(frame);
  if (keyFrameIndex >= 0) {
    setParamStep(keyFrameIndex, step,
                 stageObject->getParam(TStageObject::T_ShearX));
  }
  param         = stageObject->getParam(TStageObject::T_ShearY);
  keyFrameIndex = param->getClosestKeyframe(frame);
  if (keyFrameIndex >= 0) {
    setParamStep(keyFrameIndex, step,
                 stageObject->getParam(TStageObject::T_ShearY));
  }

  TUndoManager::manager()->endBlock();
}

//-----------------------------------------------------------------------------

void CellArea::onSetCellMark() {
  QAction *senderAction = qobject_cast<QAction *>(sender());
  assert(senderAction);
  QList<QVariant> params = senderAction->data().toList();
  assert(params.count() == 3);
  int row               = params[0].toInt();
  int col               = params[1].toInt();
  int id                = params[2].toInt();
  SetCellMarkUndo *undo = new SetCellMarkUndo(row, col, id);
  undo->redo();
  TUndoManager::manager()->add(undo);
}

//-----------------------------------------------------------------------------

}  // namespace XsheetGUI
