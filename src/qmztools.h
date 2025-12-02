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

// Returns info about the file contained in the archive
inline mz_zip_archive_file_stat za_file_stat(mz_zip_archive *pZip, int file_index)
{
    mz_zip_archive_file_stat file_stat;
    if (mz_zip_reader_file_stat(pZip, file_index, &file_stat)) {
        return file_stat;
    }

    return mz_zip_archive_file_stat();
}

// Writing or Reading: MZ_ZIP_MODE_READING = 1, MZ_ZIP_MODE_WRITING = 2
inline mz_zip_mode zipMode(void *pZip)
{
    mz_zip_archive *p = static_cast<mz_zip_archive *>(pZip);
    return p ? p->m_zip_mode : MZ_ZIP_MODE_INVALID;
}

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
