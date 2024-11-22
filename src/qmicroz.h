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

// path inside zip : data
using BufFileList = QMap<QString, QByteArray>;

// list of files (index : path) contained in the archive
using ZipContentsList = QMap<int, QString>;

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

    static bool compress_buf(const BufFileList &buf_data, const QString &zip_path);    // creates an archive with files from the listed paths and data
    static bool compress_buf(const QByteArray &data,
                             const QString &file_name, const QString &zip_path);       // creates an archive (zip_path) containing a file (file_name, data)

}; // class QMicroz

#endif // QMICROZ_H
