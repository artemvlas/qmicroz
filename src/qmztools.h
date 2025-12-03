/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Copyright (c) 2024 Artem Vlasenko
*/
#ifndef QMZTOOLS_H
#define QMZTOOLS_H

#include <QString>
#include "miniz.h"

namespace tools {
static const QChar s_sep = u'/';

// Returns "no compression" for micro files, for others by default
inline mz_uint compressLevel(qint64 data_size)
{
    return (data_size > 40) ? MZ_DEFAULT_COMPRESSION : MZ_NO_COMPRESSION;
}

// Checks whether the <name> is a sub-folder inside zip
inline bool isFolderName(const QString &name)
{
    return name.endsWith(s_sep);
}

// Checks whether the <name> is a file inside zip
inline bool isFileName(const QString &name)
{
    return !name.isEmpty() && !name.endsWith(s_sep);
}

// Appends '/' if not any
inline QString toFolderName(const QString &name)
{
    return name.endsWith(s_sep) ? name : name + s_sep;
}

} // namespace tools

#endif // QMZTOOLS_H
