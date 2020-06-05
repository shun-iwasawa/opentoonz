#include "expressionreferencemanager.h"

#include "tapp.h"

// TnzQt includes
#include "toonzqt/dvdialog.h"

// TnzLib includes
#include "toonz/txsheethandle.h"
#include "toonz/tscenehandle.h"
#include "toonz/txsheetexpr.h"
#include "toonz/doubleparamcmd.h"
#include "toonz/preferences.h"
#include "toonz/tstageobject.h"
#include "toonz/tcolumnfx.h"
#include "toonz/txshzeraryfxcolumn.h"
#include "toonz/fxdag.h"
#include "toonz/tcolumnfxset.h"
#include "toonz/toonzscene.h"
#include "toonz/txshlevelcolumn.h"
#include "toonz/txshcell.h"
#include "toonz/txshchildlevel.h"
#include "toonz/expressionreferencemonitor.h"
#include "toonz/tstageobjecttree.h"

// TnzBase includes
#include "tdoubleparam.h"
#include "texpression.h"
#include "tdoublekeyframe.h"
#include "tfx.h"

#include "tmsgcore.h"

#include <QList>

namespace {
// reference : columncommand.cpp
bool canRemoveFx(const std::set<TFx*>& leaves, TFx* fx) {
  bool removeFx = false;
  for (int i = 0; i < fx->getInputPortCount(); i++) {
    TFx* inputFx = fx->getInputPort(i)->getFx();
    if (!inputFx) continue;
    if (leaves.count(inputFx) > 0) {
      removeFx = true;
      continue;
    }
    if (!canRemoveFx(leaves, inputFx)) return false;
    removeFx = true;
  }
  return removeFx;
}

void gatherXsheets(TXsheet* xsheet, QSet<TXsheet*> & ret) {
  //既に登録済みならreturn
  if (ret.contains(xsheet)) return;

  ret.insert(xsheet);

  // シート内を走査してサブシートのXsheetを登録する
  for (int c = 0; c < xsheet->getColumnCount(); c++) {
    if (xsheet->isColumnEmpty(c)) continue;
    TXshLevelColumn *levelColumn = xsheet->getColumn(c)->getLevelColumn();
    if (!levelColumn) continue;

    int start, end;
    levelColumn->getRange(start, end);
    for (int r = start; r <= end; r++) {
      int r0, r1;
      if (!levelColumn->getLevelRange(r, r0, r1)) continue;

      TXshChildLevel* childLevel = levelColumn->getCell(r).m_level->getChildLevel();
      if (childLevel) {
        gatherXsheets(childLevel->getXsheet(), ret);
      }

      r = r1;
    }
  }
}


QSet<TXsheet*> getAllXsheets() {
  QSet<TXsheet*> ret;
  TXsheet* topXsheet = TApp::instance()->getCurrentScene()->getScene()->getTopXsheet();
  gatherXsheets(topXsheet, ret);
  return ret;
}

}  // namespace

   //----------------------------------------------------------------------------
ExpressionReferenceMonitor* ExpressionReferenceManager::currentMonitor(){
  return TApp::instance()->getCurrentXsheet()->getXsheet()->getExpRefMonitor();
}

QMap<TDoubleParam*, QSet<int>> & ExpressionReferenceManager::colRefMap(TXsheet* xsh){
  if (xsh)
    return xsh->getExpRefMonitor()->colRefMap();
  else
    return currentMonitor()->colRefMap();
}

QMap<TDoubleParam*, QSet<TDoubleParam*>> & ExpressionReferenceManager::paramRefMap(TXsheet* xsh){
  if (xsh)
    return xsh->getExpRefMonitor()->paramRefMap();
  else
    return currentMonitor()->paramRefMap();
}

QMap<TDoubleParam*, QString> & ExpressionReferenceManager::nameMap(TXsheet* xsh){
  if (xsh)
    return xsh->getExpRefMonitor()->nameMap();
  else
    return currentMonitor()->nameMap();
}

QSet<TDoubleParam*> & ExpressionReferenceManager::ignoredParamSet(TXsheet* xsh){
  if (xsh)
    return xsh->getExpRefMonitor()->ignoredParamSet();
  else
    return currentMonitor()->ignoredParamSet();
}
//-----------------------------------------------------------------------------

QSet<int>& ExpressionReferenceManager::touchColRefSet(TDoubleParam* param) {
  if (colRefMap().contains(param)) return colRefMap()[param];

  QSet<int> refSet;
  colRefMap().insert(param, refSet);
  return colRefMap()[param];
}

//-----------------------------------------------------------------------------

QSet<TDoubleParam*>& ExpressionReferenceManager::touchParamRefSet(
    TDoubleParam* param) {
  if (paramRefMap().contains(param)) return paramRefMap()[param];

  QSet<TDoubleParam*> refSet;
  paramRefMap().insert(param, refSet);
  return paramRefMap()[param];
}

//-----------------------------------------------------------------------------

ExpressionReferenceManager::ExpressionReferenceManager()
    : m_model(new FunctionTreeModel()) {}

//-----------------------------------------------------------------------------

void ExpressionReferenceManager::init() {
  connect(TApp::instance()->getCurrentScene(),
          SIGNAL(preferenceChanged(const QString&)), this,
          SLOT(onPreferenceChanged(const QString&)));
  onPreferenceChanged("modifyExpressionOnMovingColumn");
}

//-----------------------------------------------------------------------------

void ExpressionReferenceManager::onPreferenceChanged(const QString& prefName) {
  if (prefName != "modifyExpressionOnMovingColumn") return;

  TXsheetHandle* xshHandle = TApp::instance()->getCurrentXsheet();
  TSceneHandle* sceneHandle = TApp::instance()->getCurrentScene();
  bool on = Preferences::instance()->isModifyExpressionOnMovingColumnEnabled();
  if (on) {
    // when referece columns are changed, then update the reference map
    connect(xshHandle,
            SIGNAL(referenceParamsChanged(TDoubleParam*, QSet<int>, QSet<int>,
                                          QSet<TDoubleParam*>,
                                          QSet<TDoubleParam*>)),
            this,
            SLOT(onReferenceParamsChanged(TDoubleParam*, QSet<int>, QSet<int>,
                                          QSet<TDoubleParam*>,
                                          QSet<TDoubleParam*>)));
    // when the scene switched, refresh the all list
    connect(sceneHandle, SIGNAL(sceneSwitched()), this,
            SLOT(onSceneSwitched()));
    connect(xshHandle, SIGNAL(xsheetSwitched()), this,
      SLOT(onXsheetSwitched()));
    connect(xshHandle, SIGNAL(xsheetChanged()), this, SLOT(onXsheetChanged()));
    onSceneSwitched();
  } else {
    disconnect(xshHandle,
               SIGNAL(referenceParamsChanged(TDoubleParam*, QSet<int>,
                                             QSet<int>, QSet<TDoubleParam*>,
                                             QSet<TDoubleParam*>)),
               this,
               SLOT(onReferenceParamsChanged(TDoubleParam*, QSet<int>,
                                             QSet<int>, QSet<TDoubleParam*>,
                                             QSet<TDoubleParam*>)));
    // when the scene switched, refresh the all list
    disconnect(sceneHandle, SIGNAL(sceneSwitched()), this,
               SLOT(onSceneSwitched()));
    disconnect(xshHandle, SIGNAL(xsheetSwitched()), this,
      SLOT(onXsheetSwitched()));
    disconnect(xshHandle, SIGNAL(xsheetChanged()), this,
               SLOT(onXsheetChanged()));

    //全部クリアする
    QSet<TXsheet*> allXsheets = getAllXsheets();
    for (auto xsh : allXsheets) {
      xsh->getExpRefMonitor()->clearAll();
      xsh->setObserver(nullptr);
    }
  }
}

//-----------------------------------------------------------------------------

ExpressionReferenceManager* ExpressionReferenceManager::instance() {
  static ExpressionReferenceManager _instance;
  return &_instance;
}

//-----------------------------------------------------------------------------

bool ExpressionReferenceManager::refreshParamsRef(TDoubleParam* curve, TXsheet* xsh) {
  QSet<int> colRef;
  QSet<TDoubleParam*> paramsRef;
  for (int k = 0; k < curve->getKeyframeCount(); k++) {
    TDoubleKeyframe keyframe = curve->getKeyframe(k);

    if (keyframe.m_type != TDoubleKeyframe::Expression &&
        keyframe.m_type != TDoubleKeyframe::SimilarShape)
      continue;

    TExpression expr;
    expr.setGrammar(curve->getGrammar());
    expr.setText(keyframe.m_expressionText);

    QSet<int> tmpColRef;
    QSet<TDoubleParam*> tmpParamsRef;
    referenceParams(expr, tmpColRef, tmpParamsRef);
    colRef += tmpColRef;
    paramsRef += tmpParamsRef;
  }
  // replace the indices
  if (colRef.isEmpty())
    colRefMap(xsh).remove(curve);
  else
    colRefMap(xsh).insert(curve, colRef);

  if (paramsRef.isEmpty())
    paramRefMap(xsh).remove(curve);
  else
    paramRefMap(xsh).insert(curve, paramsRef);

  return !colRef.isEmpty() || !paramsRef.isEmpty();
}

//-----------------------------------------------------------------------------

void ExpressionReferenceManager::onReferenceParamsChanged(
    TDoubleParam* curve, QSet<int> colBefore, QSet<int> colAfter,
    QSet<TDoubleParam*> paramsBefore, QSet<TDoubleParam*> paramsAfter) {
  // if all items in before is contained in after, just add indices
  bool isAdd = true;
  for (auto col : colBefore) {
    if (!colAfter.contains(col)) {
      isAdd = false;
      break;
    }
  }
  if (isAdd) {
    for (auto param : paramsBefore) {
      if (!paramsAfter.contains(param)) {
        isAdd = false;
        break;
      }
    }
  }

  if (isAdd) {
    touchColRefSet(curve) += colAfter;
    touchParamRefSet(curve) += paramsAfter;
  }
  // if some references were deleted, check all keys and refresh the reference
  // list
  else {
    refreshParamsRef(curve);
  }

  FunctionTreeModel::Channel* channel = nullptr;
  for (int i = 0; i < m_model->getStageObjectsChannelCount(); i++) {
    channel = findChannel(curve, m_model->getStageObjectChannel(i));
    if (channel) break;
  }
  if (!channel) {
    for (int i = 0; i < m_model->getFxsChannelCount(); i++) {
      channel = findChannel(curve, m_model->getFxChannel(i));
      if (channel) break;
    }
  }
  if (channel) nameMap().insert(curve, channel->getLongName());
  
  // 操作されたので無視リストから解除する
  ignoredParamSet().remove(curve);
}

//-----------------------------------------------------------------------------

void ExpressionReferenceManager::checkRef(TreeModel::Item* item, TXsheet* xsh) {
  if (FunctionTreeModel::Channel* channel =
          dynamic_cast<FunctionTreeModel::Channel*>(item)) {
    TDoubleParam* curve = channel->getParam();
    bool hasRef         = refreshParamsRef(curve);
    if (hasRef) 
      nameMap(xsh).insert(curve, channel->getLongName());
    else
      nameMap(xsh).remove(curve);
  } else
    for (int i = 0; i < item->getChildCount(); i++) checkRef(item->getChild(i), xsh);
}

//-----------------------------------------------------------------------------

FunctionTreeModel::Channel* ExpressionReferenceManager::findChannel(TDoubleParam* param, TreeModel::Item* item) {
  if (FunctionTreeModel::Channel* channel =
    dynamic_cast<FunctionTreeModel::Channel*>(item)) {
    if (channel->getParam() == param)
      return channel;
  }
  else {
    for (int i = 0; i < item->getChildCount(); i++) {
      FunctionTreeModel::Channel* ret = findChannel(param, item->getChild(i));
      if (ret) return ret;
    }
  }
  return nullptr;
}

//-----------------------------------------------------------------------------

void ExpressionReferenceManager::gatherParams(TreeModel::Item* item,
                                              QSet<TDoubleParam*>& paramSet) {
  if (FunctionTreeModel::Channel* channel =
          dynamic_cast<FunctionTreeModel::Channel*>(item)) {
    paramSet.insert(channel->getParam());
  } else
    for (int i = 0; i < item->getChildCount(); i++)
      gatherParams(item->getChild(i), paramSet);
}

//-----------------------------------------------------------------------------

void ExpressionReferenceManager::onSceneSwitched() {
  QSet<TXsheet*> allXsheets = getAllXsheets();
  for (auto xsh : allXsheets) {

    xsh->setObserver(this);

    xsh->getExpRefMonitor()->clearAll();

    m_model->refreshData(xsh);
    xsh->getExpRefMonitor()->clearAll();

    for (int i = 0; i < m_model->getStageObjectsChannelCount(); i++) {
      checkRef(m_model->getStageObjectChannel(i), xsh);
    }
    for (int i = 0; i < m_model->getFxsChannelCount(); i++) {
      checkRef(m_model->getFxChannel(i), xsh);
    }
  }
  onXsheetSwitched();
}

//-----------------------------------------------------------------------------

void ExpressionReferenceManager::onXsheetSwitched() {
  TXsheet* xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
  xsh->setObserver(this);
  m_model->refreshData(xsh);
}

//-----------------------------------------------------------------------------

void ExpressionReferenceManager::removeDeletedParameters(TXsheet* xsh) {
  m_model->refreshData(xsh);

  QSet<TDoubleParam*> paramSet;
  for (int i = 0; i < m_model->getStageObjectsChannelCount(); i++) {
    gatherParams(m_model->getStageObjectChannel(i), paramSet);
  }
  for (int i = 0; i < m_model->getFxsChannelCount(); i++) {
    gatherParams(m_model->getFxChannel(i), paramSet);
  }

  // remove deleted parameter from refMap
  for (auto it = colRefMap(xsh).begin(); it != colRefMap(xsh).end();) {
    if (!paramSet.contains(it.key()))
      it = colRefMap(xsh).erase(it);
    else
      ++it;
  }

  for (auto it = ignoredParamSet(xsh).begin(); it != ignoredParamSet(xsh).end();)
    if (!paramSet.contains(*it))
      it = ignoredParamSet(xsh).erase(it);
    else
      ++it;

  for (auto it = nameMap(xsh).begin(); it != nameMap(xsh).end();)
    if (!paramSet.contains(it.key()))
      it = nameMap(xsh).erase(it);
    else
      ++it;

  for (auto it = paramRefMap(xsh).begin(); it != paramRefMap(xsh).end();) {
    if (!paramSet.contains(it.key()))
      it = paramRefMap(xsh).erase(it);
    else {
      // 次に、パラメータが消されている可能性を考慮
      if (!ignoredParamSet(xsh).contains(it.key())) {
        for (auto refParam : it.value()) {
          if (!paramSet.contains(refParam)) {
            ignoredParamSet(xsh).insert(it.key());  // 無視リストに入れる
            break;
          }
        }
      }
      ++it;
    }
  }

  TXsheet* currentXsh = TApp::instance()->getCurrentXsheet()->getXsheet();
  if(xsh != currentXsh)
    m_model->refreshData(currentXsh);
}

//----------------------------------------------------------------------------

void ExpressionReferenceManager::onXsheetChanged() {
  TXsheet* xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
  removeDeletedParameters(xsh);
}

//-----------------------------------------------------------------------------

void ExpressionReferenceManager::onChange(const TXsheetColumnChange& change) {
  TXsheet* xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
  QMap<int, int> replaceTable;
  switch (change.m_type) {
  case TXsheetColumnChange::Insert: {
    for (int c = xsh->getColumnCount() - 2; c >= change.m_index1; c--) {
      replaceTable.insert(c, c + 1);
    }
  } break;
  case TXsheetColumnChange::Remove: {
    // 監視除外リストを更新
    for (auto it = colRefMap().begin(); it != colRefMap().end(); it++) {
      if (it.value().contains(change.m_index1))
        ignoredParamSet().insert(it.key());
    }
    // checkReferenceRemoval(change.m_index1);
    for (int c = change.m_index1; c < xsh->getColumnCount(); c++) {
      replaceTable.insert(c + 1, c);
    }
  } break;
  case TXsheetColumnChange::Move: {
    if (change.m_index1 < change.m_index2) {
      replaceTable.insert(change.m_index1, change.m_index2);
      for (int c = change.m_index1 + 1; c <= change.m_index2; c++) {
        replaceTable.insert(c, c - 1);
      }
    } else {
      replaceTable.insert(change.m_index1, change.m_index2);
      for (int c = change.m_index2; c < change.m_index1; c++) {
        replaceTable.insert(c, c + 1);
      }
    }
  } break;
  }

  doColumnReplace(replaceTable, xsh);
}

void ExpressionReferenceManager::onFxAdded(const std::vector<TFx*>& fxs) {
  for (int i = 0; i < m_model->getFxsChannelCount(); i++) {
    FxChannelGroup* fcg =
        dynamic_cast<FxChannelGroup*>(m_model->getFxChannel(i));
    if (fcg && fxs.end() != std::find(fxs.begin(), fxs.end(), fcg->getFx()))
      checkRef(fcg);
  }
}

void ExpressionReferenceManager::onStageObjectAdded(
    const TStageObjectId objId) {
  for (int i = 0; i < m_model->getStageObjectsChannelCount(); i++) {
    StageObjectChannelGroup* socg = dynamic_cast<StageObjectChannelGroup*>(
        m_model->getStageObjectChannel(i));
    if (socg && objId == socg->getStageObject()->getId()) checkRef(socg);
  }
}

bool ExpressionReferenceManager::isIgnored(TDoubleParam* param) {
  return ignoredParamSet().contains(param);
}

//-----------------------------------------------------------------------------

void ExpressionReferenceManager::doColumnReplace(
    const QMap<int, int> replaceTable, TXsheet* xsh) {
  for (auto it = colRefMap(xsh).begin(); it != colRefMap(xsh).end(); it++) {
    TDoubleParam* curve = it.key();
    if (ignoredParamSet(xsh).contains(curve)) {
      for (int kIndex = 0; kIndex < curve->getKeyframeCount(); kIndex++) {
        TDoubleKeyframe keyframe = curve->getKeyframe(kIndex);
        if (keyframe.m_type != TDoubleKeyframe::Expression &&
            keyframe.m_type != TDoubleKeyframe::SimilarShape)
          continue;

        //循環参照をチェックする
        TExpression expr;
        expr.setGrammar(curve->getGrammar());
        expr.setText(keyframe.m_expressionText);        
        //循環参照が発生した場合はカラム番号を`?n?`に差し替える
        if (dependsOn(expr, curve)) {
          // replace expression
          QString expr = QString::fromStdString(keyframe.m_expressionText);
          QStringList list = expr.split('"');
          bool isStringToken = false;
          for (QString& partialExp : list) {
            if (isStringToken) continue;
            isStringToken = !isStringToken;
            int j = 0;
            while ((j = partialExp.indexOf("col", j)) != -1) {
              // move iterator to the column number
              j += 3;
              // position to the next period
              int k = partialExp.indexOf(".", j);
              if (k != -1) {
                // obtain column number
                QString numStr = partialExp.mid(j, k - j);
                bool ok;
                int colIndex = numStr.toInt(&ok);
                if (ok) {
                  partialExp.replace(
                    j, k - j,"?"+ numStr +"?");
                }
              }
              ++j;
            }
          }

          QString newExpr = list.join('"');
          keyframe.m_expressionText = newExpr.toStdString();
        }

        KeyframeSetter setter(curve, kIndex, false);
        if (keyframe.m_type == TDoubleKeyframe::Expression)
          setter.setExpression(keyframe.m_expressionText);
        else  // SimilarShape case
          setter.setSimilarShape(keyframe.m_expressionText,
                                 keyframe.m_similarShapeOffset);
      }
      continue;
    }

    QSet<int> refs = it.value();
    for (int idToBeReplaced : replaceTable.keys()) {
      if (refs.contains(idToBeReplaced)) {  // found the curve to be replaced
        for (int kIndex = 0; kIndex < curve->getKeyframeCount(); kIndex++) {
          TDoubleKeyframe keyframe = curve->getKeyframe(kIndex);

          if (keyframe.m_type != TDoubleKeyframe::Expression &&
              keyframe.m_type != TDoubleKeyframe::SimilarShape)
            continue;

          // replace expression
          QString expr     = QString::fromStdString(keyframe.m_expressionText);
          QStringList list = expr.split('"');
          bool isStringToken = false;
          for (QString& partialExp : list) {
            if (isStringToken) continue;
            isStringToken = !isStringToken;
            int j         = 0;
            while ((j = partialExp.indexOf("col", j)) != -1) {
              // move iterator to the column number
              j += 3;
              // position to the next period
              int k = partialExp.indexOf(".", j);
              if (k != -1) {
                // obtain column number
                QString numStr = partialExp.mid(j, k - j);
                bool ok;
                int colIndex = numStr.toInt(&ok);
                if (ok && replaceTable.contains(colIndex - 1)) {
                  partialExp.replace(
                      j, k - j,
                      QString::number(replaceTable.value(colIndex - 1) + 1));
                }
              }
              ++j;
            }
          }

          QString newExpr = list.join('"');

          KeyframeSetter setter(curve, kIndex, false);
          if (keyframe.m_type == TDoubleKeyframe::Expression)
            setter.setExpression(newExpr.toStdString());
          else  // SimilarShape case
            setter.setSimilarShape(newExpr.toStdString(),
                                   keyframe.m_similarShapeOffset);

          DVGui::info(
              tr("Expression modified: \"%1\" key at frame %2, %3 -> %4")
                  .arg(nameMap(xsh).value(curve))
                  .arg(keyframe.m_frame + 1)
                  .arg(expr)
                  .arg(newExpr));
        }

        QSet<int> newSet;
        for (int index : refs) {
          if (replaceTable.contains(index))
            newSet.insert(replaceTable.value(index));
          else
            newSet.insert(index);
        }
        colRefMap(xsh).insert(curve, newSet);
      }
    }
  }
}

//-----------------------------------------------------------------------------
bool ExpressionReferenceManager::doCheckReferenceDeletion(
    const QSet<int>& columnIdsToBeDeleted, const QSet<TFx*>& fxsToBeDeleted,
    const QList<TStageObjectId>& objectIdsToBeDeleted,
  const QList<TStageObjectId>& objIdsToBeDuplicated, bool checkInvert) {
  // これから消えてしまうパラメータを参照しているパラメータ一覧
  QSet<TDoubleParam*> cautionParams;
  QSet<TDoubleParam*> invCautionParams;

  // columnIdを含むparamを見つける
  for (auto it = colRefMap().begin(); it != colRefMap().end(); it++) {
    for (auto refColId : it.value()) {
      if (columnIdsToBeDeleted.contains(refColId))
        cautionParams.insert(it.key());
      else if (checkInvert)
        invCautionParams.insert(it.key());
    }
  }

  // Fxのパラメータを参照しているparamを見つける
  QSet<TDoubleParam*> fxParamsToBeDeleted;
  QSet<TDoubleParam*> invFxParamsToBeDeleted;
  for (int i = 0; i < m_model->getFxsChannelCount(); i++) {
    FxChannelGroup* fcg =
      dynamic_cast<FxChannelGroup*>(m_model->getFxChannel(i));
    if (!fcg) continue;
    if(fxsToBeDeleted.contains(fcg->getFx()))
      gatherParams(fcg, fxParamsToBeDeleted);
    else if(checkInvert)
      gatherParams(fcg, invFxParamsToBeDeleted);
  }
  for (auto it = paramRefMap().begin(); it != paramRefMap().end(); it++) {
    for (auto refParam : it.value()) {
      if (fxParamsToBeDeleted.contains(refParam)) 
        cautionParams.insert(it.key());
      if (checkInvert && invFxParamsToBeDeleted.contains(refParam))
        invCautionParams.insert(it.key());
    }
  }

  // Stageのパラメータを参照しているparamを見つける
  QSet<TDoubleParam*> stageParamsToBeDeleted;
  QSet<TDoubleParam*> invStageParamsToBeDeleted;
  for (int i = 0; i < m_model->getStageObjectsChannelCount(); i++) {
    StageObjectChannelGroup* socg = dynamic_cast<StageObjectChannelGroup*>(
      m_model->getStageObjectChannel(i));
    if (!socg) continue;
    TStageObjectId id = socg->getStageObject()->getId();
    if(objectIdsToBeDeleted.contains(id))
      gatherParams(socg, stageParamsToBeDeleted);
    // サブシート内に複製されるオブジェクトは参照が切れない
    else if( checkInvert && !objIdsToBeDuplicated.contains(id) )
      gatherParams(socg, invStageParamsToBeDeleted);
  }
  for (auto it = paramRefMap().begin(); it != paramRefMap().end(); it++) {
    for (auto refParam : it.value()) {
      if (stageParamsToBeDeleted.contains(refParam))
        cautionParams.insert(it.key());
      if (checkInvert && invStageParamsToBeDeleted.contains(refParam))
        invCautionParams.insert(it.key());
    }
  }

  //パラメータ一覧の中で、それら自体が消える場合は除外
  for (auto it = cautionParams.begin(); it != cautionParams.end();)
    if (fxParamsToBeDeleted.contains(*it) ||
      stageParamsToBeDeleted.contains(*it))
      it = cautionParams.erase(it);
    else
      ++it;
  for (auto it = invCautionParams.begin(); it != invCautionParams.end();)
    if (invFxParamsToBeDeleted.contains(*it) ||
      invStageParamsToBeDeleted.contains(*it))
      it = invCautionParams.erase(it);
    else
      ++it;
  

  //警告が必要なパラメータがある場合はポップアップを出す
  if (cautionParams.isEmpty() && invCautionParams.isEmpty()) return true;

  QString warningTxt =
      tr("Following parameters will lose reference in expressions:");
  for (auto param : cautionParams) {
    warningTxt += "\n  " + nameMap().value(param);
  }
  for (auto param : invCautionParams) {
    warningTxt += "\n  " + nameMap().value(param) + "  " + tr("(To be in the subxsheet)");
  }
  warningTxt += tr("\nDo you want to delete anyway ?");

  int ret =
      DVGui::MsgBox(warningTxt, QObject::tr("OK"), QObject::tr("Cancel"), 0);
  if (ret == 0 || ret == 2) return false;

  return true;
}

//-----------------------------------------------------------------------------
// check on deleting columns
bool ExpressionReferenceManager::checkReferenceDeletion(
    const QSet<int>& columnIdsToBeDeleted, 
  const QSet<TFx*>& fxsToBeDeleted,
  const QList<TStageObjectId>& objIdsToBeDuplicated,
  bool checkInvert) {
  QList<TStageObjectId> objectIdsToBeDeleted;
  for (auto colId : columnIdsToBeDeleted)
    objectIdsToBeDeleted.append(TStageObjectId::ColumnId(colId));

  return doCheckReferenceDeletion(columnIdsToBeDeleted, fxsToBeDeleted,
                                  objectIdsToBeDeleted,
                                  objIdsToBeDuplicated, checkInvert);
}

//-----------------------------------------------------------------------------
// check on deleting stage objects
bool ExpressionReferenceManager::checkReferenceDeletion(
    const QList<TStageObjectId>& objectIdsToBeDeleted) {
  QSet<int> columnIdsToBeDeleted;
  QSet<TFx*> fxsToBeDeleted;

  TApp* app    = TApp::instance();
  TXsheet* xsh = app->getCurrentXsheet()->getXsheet();
  std::set<TFx*> leaves;
  // カラムの場合はFxの参照も確認も必要
  for (const auto& objId : objectIdsToBeDeleted) {
    if (objId.isColumn()) {
      int index = objId.getIndex();
      if (index < 0) continue;
      TXshColumn* column = xsh->getColumn(index);
      if (!column) continue;
      columnIdsToBeDeleted.insert(index);
      TFx* fx = column->getFx();
      if (fx) {
        leaves.insert(fx);
        TZeraryColumnFx* zcfx = dynamic_cast<TZeraryColumnFx*>(fx);
        if (zcfx) fxsToBeDeleted.insert(zcfx->getZeraryFx());
      }
    }
  }
  /*-- カラムを消した時、一緒に消してもよいFxを格納していく --*/
  TFxSet* fxSet = xsh->getFxDag()->getInternalFxs();
  for (int i = 0; i < fxSet->getFxCount(); i++) {
    TFx* fx = fxSet->getFx(i);
    if (canRemoveFx(leaves, fx)) fxsToBeDeleted.insert(fx);
  }
  QList<TStageObjectId> dummy;

  return doCheckReferenceDeletion(columnIdsToBeDeleted, fxsToBeDeleted,
                                  objectIdsToBeDeleted, dummy);
}

//----------------------------------------------------------------------------
// xhseet / fx schematic上でcollapseをした場合の処理

void ExpressionReferenceManager::onCollapse(TXsheet* childXsh,
  ExpressionReferenceMonitor* parentMonitor,
  std::set<int> indices, std::set<int> newIndices,
  bool columnsOnly) {
  // PreferenceがOFFならreturn
  bool on = Preferences::instance()->isModifyExpressionOnMovingColumnEnabled();
  if (!on) return;
  QList<TStageObjectId> duplicatedObjs;
  // pegbarを持ち込む場合は、サブシート内に複製されたオブジェクトのパラメータにポインタを差し替えて監視を続行する
  if (!columnsOnly) {
    for (auto index : newIndices) {
      TStageObjectId id =
        childXsh->getStageObjectParent(TStageObjectId::ColumnId(index));
      while (id.isPegbar() || id.isCamera()) {
        duplicatedObjs.append(id);
        id = childXsh->getStageObjectParent(id);
      }
    }
  }
  doOnCollapse(childXsh, parentMonitor, indices, newIndices, duplicatedObjs);
}

//----------------------------------------------------------------------------
// stage schematic 上でcollapseをした場合の処理

void ExpressionReferenceManager::onCollapse(TXsheet* childXsh,
  ExpressionReferenceMonitor* parentMonitor,
  std::set<int> indices, std::set<int> newIndices, const QList<TStageObjectId> &objIds) {
  // PreferenceがOFFならreturn
  bool on = Preferences::instance()->isModifyExpressionOnMovingColumnEnabled();
  if (!on) return;
  QList<TStageObjectId> duplicatedObjs;
  for (auto id : objIds) {
    if (id.isPegbar() || id.isCamera())
      duplicatedObjs.append(id);
  }
  
  doOnCollapse(childXsh, parentMonitor, indices, newIndices, duplicatedObjs);
}

  //----------------------------------------------------------------------------
void ExpressionReferenceManager::doOnCollapse(TXsheet* childXsh,
  ExpressionReferenceMonitor* parentMonitor,
  std::set<int> indices, std::set<int> newIndices, 
  QList<TStageObjectId>& duplicatedObjs) {
    
  //まずコピー
  colRefMap(childXsh) = parentMonitor->colRefMap();
  paramRefMap(childXsh) = parentMonitor->paramRefMap();
  nameMap(childXsh) = parentMonitor->nameMap();
  ignoredParamSet(childXsh) = parentMonitor->ignoredParamSet();

  // pegbarを持ち込む場合は、サブシート内に複製されたオブジェクトのパラメータにポインタを差し替えて監視を続行する
  if (!duplicatedObjs.isEmpty()) {

    // 差し替えテーブルを作る
    QMap<TDoubleParam*, TDoubleParam*> replaceTable;
    TApp* app = TApp::instance();
    TXsheet* xsh = app->getCurrentXsheet()->getXsheet();
    for ( auto dupObjId : duplicatedObjs){
      TStageObject *parentObj = xsh->getStageObjectTree()->getStageObject(dupObjId, false);
      TStageObject *childObj = childXsh->getStageObjectTree()->getStageObject(dupObjId, false);
      assert(parentObj && childObj);
      if (!parentObj || !childObj) continue;
      for (int c = 0; c < TStageObject::T_ChannelCount; c++) {
        TDoubleParam* parent_p = parentObj->getParam((TStageObject::Channel)c);
        TDoubleParam* child_p = childObj->getParam((TStageObject::Channel)c);
        replaceTable.insert(parent_p, child_p);
      }
    }

    //差し替える
    QMap<TDoubleParam*, TDoubleParam*>::const_iterator table_itr = replaceTable.constBegin();
    while (table_itr != replaceTable.constEnd()) {
      //cout << i.key() << ": " << i.value() << endl;
      auto colRef_itr = colRefMap(childXsh).find(table_itr.key());
      if (colRef_itr != colRefMap(childXsh).end()) {
        QSet<int> val = colRef_itr.value();
        colRefMap(childXsh).erase(colRef_itr);
        colRefMap(childXsh).insert(table_itr.value(), val);
      }
      // keyの差し替え。valueの方は後で行う
      auto paramRef_itr = paramRefMap(childXsh).find(table_itr.key());
      if (paramRef_itr != paramRefMap(childXsh).end()) {
        QSet<TDoubleParam*> val = paramRef_itr.value();
        paramRefMap(childXsh).erase(paramRef_itr);
        paramRefMap(childXsh).insert(table_itr.value(), val);
      }
      auto name_itr = nameMap(childXsh).find(table_itr.key());
      if (name_itr != nameMap(childXsh).end()) {
        QString val = name_itr.value();
        nameMap(childXsh).erase(name_itr);
        nameMap(childXsh).insert(table_itr.value(), val);
      }
      auto ignore_itr = ignoredParamSet(childXsh).find(table_itr.key());
      if (ignore_itr != ignoredParamSet(childXsh).end()) {
        ignoredParamSet(childXsh).erase(ignore_itr);
        ignoredParamSet(childXsh).insert(table_itr.value());
      }
      ++table_itr;
    }
    //paramRefMapのvalueの方の差し替え
    QMap<TDoubleParam*, QSet<TDoubleParam*>>::iterator pRef_itr = paramRefMap(childXsh).begin();
    while (pRef_itr != paramRefMap(childXsh).end()) {
      QSet<TDoubleParam*> repSet;
      QSet<TDoubleParam*>::iterator i = pRef_itr.value().begin();
      while (i != pRef_itr.value().end()) {
        if (replaceTable.contains(*i)) {
          repSet.insert(replaceTable.value(*i));
          i = pRef_itr.value().erase(i);
        }
        else
          ++i;
      }
      pRef_itr.value().unite(repSet);

      ++pRef_itr;
    }

  }

  // parentMonitorの中から、畳まれたxshに含まれる項目を移行
  // ignore項目を更新
  removeDeletedParameters(childXsh);
  
  // column indices がcollapse範囲に含まれなかった項目をignoreリストに追加する
  for (auto it = colRefMap(childXsh).begin(); it != colRefMap(childXsh).end();) {
    for (auto col: it.value()) {
      if (indices.find(col) == indices.end()) { // 参照カラムがたたむインデックス一覧に含まれていない場合
        ignoredParamSet(childXsh).insert(it.key()); // 無視リストに入れる
        break;
      }
    }
    
    ++it;
  }

  // ColumnIndicesのシフト
  QMap<int, int> replaceTable;
  assert(indices.size() == newIndices.size());
  auto toItr = newIndices.begin();
  for (auto fromItr = indices.begin(); fromItr != indices.end(); ++fromItr, ++toItr) {
    replaceTable.insert(*fromItr, *toItr);
  }
  doColumnReplace(replaceTable, childXsh);
}