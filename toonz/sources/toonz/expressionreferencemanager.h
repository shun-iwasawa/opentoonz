#pragma once

#ifndef EXPRESSIONREFERENCEMANAGER_H
#define EXPRESSIONREFERENCEMANAGER_H

#include "toonzqt/treemodel.h"
#include "toonzqt/functiontreeviewer.h"
#include "toonz/txsheetcolumnchange.h"
#include "toonz/tstageobjectid.h"
#include <QObject>
#include <QMap>
#include <QSet>
#include <QList>

class TDoubleParam;
class TFx;
class ExpressionReferenceMonitor;

class ExpressionReferenceManager
    : public QObject,
      public TXsheetColumnChangeObserver {  // singleton
  Q_OBJECT

  FunctionTreeModel* m_model;
  
  ExpressionReferenceMonitor* currentMonitor();
  QMap<TDoubleParam*, QSet<int>> & colRefMap(TXsheet* xsh = nullptr);
  QMap<TDoubleParam*, QSet<TDoubleParam*>> & paramRefMap(TXsheet* xsh = nullptr);
  QMap<TDoubleParam*, QString> & nameMap(TXsheet* xsh = nullptr);
  QSet<TDoubleParam*> & ignoredParamSet(TXsheet* xsh = nullptr);

  QSet<int>& touchColRefSet(TDoubleParam*);
  QSet<TDoubleParam*>& touchParamRefSet(TDoubleParam*);
  ExpressionReferenceManager();
  
  void checkRef(TreeModel::Item* item, TXsheet* xsh = nullptr);
  FunctionTreeModel::Channel* findChannel(TDoubleParam* param, TreeModel::Item* item);

  void gatherParams(TreeModel::Item* item, QSet<TDoubleParam*>&);
  bool refreshParamsRef(TDoubleParam* curve, TXsheet* xsh = nullptr);
  void doColumnReplace(const QMap<int, int> replaceTable, TXsheet* xsh);
  // void checkReferenceRemoval(int indexToBeRemoved);

  bool doCheckReferenceDeletion(
      const QSet<int>& columnIdsToBeDeleted, const QSet<TFx*>& fxsToBeDeleted,
      const QList<TStageObjectId>& objectIdsToBeDeleted, bool checkInvert = false);

  void removeDeletedParameters(TXsheet* xsh);

public:
  static ExpressionReferenceManager* instance();
  void onChange(const TXsheetColumnChange&) override;
  void onFxAdded(const std::vector<TFx*>&) override;
  void onStageObjectAdded(const TStageObjectId) override;
  bool isIgnored(TDoubleParam*) override;
  void init();

  bool checkReferenceDeletion(const QSet<int>& columnIdsToBeDeleted,
                              const QSet<TFx*>& fxsToBeDeleted, bool checkInvert);
  bool checkReferenceDeletion(
      const QList<TStageObjectId>& objectIdsToBeDeleted);

  void onCollapse(TXsheet* childXsh, ExpressionReferenceMonitor* parentMonitor,
    std::set<int> indices, std::set<int> newIndices);
protected slots:
  void onReferenceParamsChanged(TDoubleParam* curve, QSet<int> colBefore,
                                QSet<int> colAfter,
                                QSet<TDoubleParam*> paramsBefore,
                                QSet<TDoubleParam*> paramsAfter);
  void onSceneSwitched();
  void onXsheetSwitched();
  void onXsheetChanged();
  void onPreferenceChanged(const QString& prefName);
};

#endif