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
enum ZaType { ZaReader, ZaWriter };

// creates and initializes a new archive
mz_zip_archive* za_new(const QString &zip_path, ZaType za_type);

// returns info about the file contained in the archive
mz_zip_archive_file_stat za_file_stat(mz_zip_archive *pZip, int file_index);

// closes and deletes the archive
bool za_close(mz_zip_archive *pZip);

// adds to the archive an entry with file or folder data
bool add_item_data(mz_zip_archive *pZip, const QString &item_path, const QByteArray &data);

// adds an empty subfolder item to the zip; <item_path> is the path/name inside the archive
bool add_item_folder(mz_zip_archive *pZip, const QString &item_path);

// adds file item and data to zip; 'fs_path' is the filesystem path; 'item_path' is the path inside the archive
bool add_item_file(mz_zip_archive *pZip, const QString &fs_path, const QString &item_path);

// parses the list of file/folder paths and adds them to the archive
bool add_item_list(mz_zip_archive *pZip, const QStringList &items, const QString &rootFolder, bool verbose = false);

// extracts a file with the specified index from the archive to disk at the specified path
bool extract_to_file(mz_zip_archive *pZip, int file_index, const QString &outpath);

// extracts file data at the specified index into a buffer
QByteArray extract_to_buffer(mz_zip_archive *pZip, int file_index, bool copy_data = true);

// extracts the entire contents of the archive into the specified folder
bool extract_all_to_disk(mz_zip_archive *pZip, const QString &output_folder, bool verbose = false);

// returns a path list of the folder content: files and subfolders
QStringList folderContent(const QString &folder);

// Concatenates path strings, checking for the presence of a separator
QString joinPath(const QString &abs_path, const QString &rel_path);

/* Creates a folder at the specified <path>
 * If already exists or created successfully, returns true; otherwise false
 */
bool createFolder(const QString &path);


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
