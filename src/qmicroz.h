/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Author: Artem Vlasenko
 * https://github.com/artemvlas
*/

#ifndef QMICROZ_H
#define QMICROZ_H

#include "qmicroz_global.h"
#include <QStringList>

class QMICROZ_EXPORT QMicroz
{
public:
    QMicroz();

    static bool extract(const QString &zip_path);                                      // extracting the zip archive into the parent dir
    static bool extract(const QString &zip_path, const QString &output_folder);        // to output_folder

    static bool compress_(const QString &path);                                        // archiving a file or folder (path), >> parent dir
    static bool compress_(const QStringList &paths);                                   // paths to files or/and folders
    static bool compress_file(const QString &source_path);                             // archiving a file, >> parent dir
    static bool compress_file(const QString &source_path, const QString &zip_path);    // >> zip_path
    static bool compress_folder(const QString &source_path);                           // archiving a folder, >> parent dir
    static bool compress_folder(const QString &source_path, const QString &zip_path);  // >> zip_path
    static bool compress_list(const QStringList &paths, const QString &zip_path);      // archiving a list of files or folders (paths), >> zip_path

}; // class QMicroz

#endif // QMICROZ_H
