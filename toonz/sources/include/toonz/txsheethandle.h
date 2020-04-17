#pragma once

#ifndef TXSHEETHANDLE_H
#define TXSHEETHANDLE_H

#include <QObject>
#include <QSet>

#include "tcommon.h"

#undef DVAPI
#undef DVVAR
#ifdef TOONZLIB_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

// forward declaration
class TXsheet;
class TDoubleParam;

//=============================================================================
// TXsheetHandle
//-----------------------------------------------------------------------------

class DVAPI TXsheetHandle final : public QObject {
  Q_OBJECT

  TXsheet *m_xsheet;

public:
  TXsheetHandle();
  ~TXsheetHandle();

  TXsheet *getXsheet() const;
  void setXsheet(TXsheet *xsheet);
  void notifyXsheetChanged() { emit xsheetChanged(); }
  void notifyXsheetSwitched() { emit xsheetSwitched(); }
  void notifyXsheetSoundChanged() { emit xsheetSoundChanged(); }
  void changeXsheetCamera(int index) { emit xsheetCameraChange(index); }

  void notifyReferenceParamsChanged(TDoubleParam *curve, QSet<int> colBefore,
                                    QSet<int> colAfter,
                                    QSet<TDoubleParam *> paramsBefore,
                                    QSet<TDoubleParam *> paramsAfter) {
    emit referenceParamsChanged(curve, colBefore, colAfter, paramsBefore,
                                paramsAfter);
  }

signals:
  void xsheetSwitched();
  void xsheetChanged();
  void xsheetSoundChanged();
  void xsheetCameraChange(int);

  void referenceParamsChanged(TDoubleParam *curve, QSet<int> colBefore,
                              QSet<int> colAfter,
                              QSet<TDoubleParam *> paramsBefore,
                              QSet<TDoubleParam *> paramsAfter);
};

#endif  // TXSHEETHANDLE_H
