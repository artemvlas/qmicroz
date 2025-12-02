/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Copyright (c) 2024 Artem Vlasenko
*/
#include "qmztools.h"
#include <QDebug>
#include <QStringBuilder>

namespace tools {
mz_zip_archive_file_stat za_file_stat(void *pZip, int file_index)
{
    mz_zip_archive *p = static_cast<mz_zip_archive *>(pZip);

    mz_zip_archive_file_stat file_stat;
    if (mz_zip_reader_file_stat(p, file_index, &file_stat)) {
        return file_stat;
    }

    return mz_zip_archive_file_stat();
}

QString joinPath(const QString &abs_path, const QString &rel_path)
{
    auto isSep = [] (QChar ch) { return ch == '/' || ch == '\\'; };

    const bool s1Ends = !abs_path.isEmpty() && isSep(abs_path.back());
    const bool s2Starts = !rel_path.isEmpty() && isSep(rel_path.front());

    if (s1Ends && s2Starts) {
        QStringView chopped = QStringView(abs_path).left(abs_path.size() - 1);
        return chopped % rel_path;
    }

    if (s1Ends || s2Starts)
        return abs_path + rel_path;

    return abs_path % s_sep % rel_path;
}

} // namespace tools
