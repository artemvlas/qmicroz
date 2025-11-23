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
#include <QDateTime>
#include "miniz.h"

namespace tools {
static const QChar s_sep = u'/';
enum ZaType { ZaReader, ZaWriter };

// Creates and initializes a new archive
mz_zip_archive* za_new(const QString &zip_path, ZaType za_type);

// Returns info about the file contained in the archive
mz_zip_archive_file_stat za_file_stat(void *pZip, int file_index);

// Closes and deletes the archive
bool za_close(mz_zip_archive *pZip);

/* Adds to the archive a file or folder entry.
 * <item_path> is a name/path inside the archive, <data> is the file data.
 * To add a folder entry, <item_path> should end with a slash ('/'), and the <data> array must be empty.
 * Or just use <add_item_folder> function.
 */
bool add_item_data(mz_zip_archive *pZip, const QString &item_path, const QByteArray &data);
bool add_item_data(mz_zip_archive *pZip, const QString &item_path, const QByteArray &data,
                   const QDateTime &lastModified);

/* Adds a folder entry (an empty subfolder) to the zip.
 * <item_path> is the folder name/path inside the archive.
 * Appending slash to the <item_path> string is NOT required (automatically).
 */
bool add_item_folder(mz_zip_archive *pZip, const QString &item_path);

/* Adds a file entry and its data to the zip.
 * <fs_path> is the filesystem path; <item_path> is the name/path inside the archive.
 */
bool add_item_file(mz_zip_archive *pZip, const QString &fs_path, const QString &item_path);

// Extracts a file with the specified index from the archive to disk at the specified path
bool extract_to_file(mz_zip_archive *pZip, int file_index, const QString &outpath);

// Extracts file data at the specified index into a buffer
QByteArray extract_to_buffer(mz_zip_archive *pZip, int file_index, bool copy_data = true);

// Returns a path list of the <folder> content: files and subfolders
QStringList folderContent(const QString &folder);

// ... { "full path" : "relative path" }
QMap<QString, QString> folderContentRel(const QString &folder);

// Concatenates path strings, checking for the presence of a separator
QString joinPath(const QString &abs_path, const QString &rel_path);

/* Creates a folder at the specified <path>
 * If already exists or created successfully, returns true; otherwise false
 */
bool createFolder(const QString &path);

// Appends '/' if not any
inline QString toFolderName(const QString &name)
{
    return name.endsWith(s_sep) ? name : name + s_sep;
}

// Writing or Reading: MZ_ZIP_MODE_READING = 1, MZ_ZIP_MODE_WRITING = 2
inline mz_zip_mode zipMode(void *pZip)
{
    mz_zip_archive *p = static_cast<mz_zip_archive *>(pZip);
    return p ? p->m_zip_mode : MZ_ZIP_MODE_INVALID;
}

// Returns the name (path) of a file/folder in the archive at the specified index
inline QString za_item_name(mz_zip_archive *pZip, int file_index)
{
    return za_file_stat(pZip, file_index).m_filename;
}

// Returns "no compression" for micro files, for others by default
inline mz_uint compressLevel(qint64 data_size)
{
    return (data_size > 40) ? MZ_DEFAULT_COMPRESSION : MZ_NO_COMPRESSION;
}

// Checks whether the <item_path> is a sub-folder inside zip
inline bool isFolderItem(const QString &item_path)
{
    return item_path.endsWith(s_sep);
}

// Checks whether the <item_path> is a file inside zip
inline bool isFileItem(const QString &item_path)
{
    return !item_path.isEmpty() && !item_path.endsWith(s_sep);
}

} // namespace tools

#endif // QMZTOOLS_H
