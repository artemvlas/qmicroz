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
#include <QMap>

// path inside zip : data
using BufFileList = QMap<QString, QByteArray>;

struct BufFile {
    BufFile() {}
    BufFile(const QString &filename, const QByteArray &data)
        : m_name(filename), m_data(data) {}

    QString m_name;
    QByteArray m_data;
};

// list of files (index : path) contained in the archive
using ZipContentsList = QMap<int, QString>;

class QMICROZ_EXPORT QMicroz
{
public:
    QMicroz();
    QMicroz(const QString &zip_path);
    ~QMicroz();

    bool setZipFile(const QString &zip_path);                                          // sets and opens a zip archive for the current object
    const ZipContentsList& zipContents();                                              // returns a list of files contained in a given archive
    bool extractAll(const QString &output_folder) const;                               // extracts the archive into the specified folder
    bool extractFile(const QString &file_path, const QString &output_folder);          // extracts the specified files
    bool extractFile(const int file_index, const QString &output_folder) const;        // by index

    BufFileList extract_to_ram() const;                                                // extracts the archive into the RAM buffer
    BufFile     extract_to_ram(const QString &file_name);                              // extracts selected file with name
    BufFile     extract_to_ram(const int file_index) const;                            // with the specified index

    // STATIC functions
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

private:
    int findIndex(const QString &path);                                                // finds the file index by the specified name
    const ZipContentsList& updateZipContents();
    bool closeArchive();

    // the void pointer is used to allow the miniz header not to be included
    void *m_archive = nullptr;

    QString m_zip_path;
    ZipContentsList m_zip_contents;

}; // class QMicroz

#endif // QMICROZ_H
