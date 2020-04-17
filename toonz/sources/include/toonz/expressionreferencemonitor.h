#pragma once

#ifndef EXPRESSIONREFERENCEMONITOR_H
#define EXPRESSIONREFERENCEMONITOR_H

#undef DVAPI
#undef DVVAR
#ifdef TOONZLIB_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

#include <QMap>
#include <QSet>
#include <QString>

class TDoubleParam;

class DVAPI ExpressionReferenceMonitor {
  QMap<TDoubleParam*, QSet<int>> m_colRefMap;
  QMap<TDoubleParam*, QSet<TDoubleParam*>> m_paramRefMap;
  QMap<TDoubleParam*, QString> m_nameMap;
  QSet<TDoubleParam*> m_ignoredParamSet;

public:
  ExpressionReferenceMonitor() {}
  QMap<TDoubleParam*, QSet<int>> & colRefMap() { return m_colRefMap; }
  QMap<TDoubleParam*, QSet<TDoubleParam*>> & paramRefMap() { return m_paramRefMap; }
  QMap<TDoubleParam*, QString> & nameMap() { return m_nameMap; }
  QSet<TDoubleParam*> & ignoredParamSet() { return m_ignoredParamSet; }
  
  void clearAll() {
    m_colRefMap.clear();
    m_paramRefMap.clear();
    m_nameMap.clear();
    m_ignoredParamSet.clear();  
  }

  ExpressionReferenceMonitor* clone()
  {
    ExpressionReferenceMonitor* ret = new ExpressionReferenceMonitor();
    ret->colRefMap() = m_colRefMap;
    ret->paramRefMap() = m_paramRefMap;
    ret->nameMap() = m_nameMap;
    ret->ignoredParamSet() = m_ignoredParamSet;
    return ret;
  }
};




#endif
