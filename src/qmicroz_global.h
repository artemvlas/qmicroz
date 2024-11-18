#ifndef QMICROZ_GLOBAL_H
#define QMICROZ_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QMICROZ_LIBRARY)
#define QMICROZ_EXPORT Q_DECL_EXPORT
#else
#define QMICROZ_EXPORT Q_DECL_IMPORT
#endif

#endif // QMICROZ_GLOBAL_H
