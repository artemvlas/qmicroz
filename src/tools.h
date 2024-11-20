/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Author: Artem Vlasenko
 * https://github.com/artemvlas
*/
#ifndef TOOLS_H
#define TOOLS_H

#include <QString>
#include "miniz.h"

namespace tools {
static const QChar s_sep = u'/';

// creates an archive at the specified path and adds a list of files and folders to it
// root is the part of the path relative to which paths in the archive will be created
bool createArchive(const QString &zip_path, const QStringList &item_paths, const QString &zip_root);

// adds file or folder data to the archive
bool addItemToZip(mz_zip_archive *p_zip, const QStringList &items, const QString &rootFolder);

// parses the list of file/folder paths and adds them to the archive
bool addItemsToZip(mz_zip_archive *p_zip, const QStringList &items, const QString &rootFolder);

// adds an empty subfolder item to the zip; 'in_path' is the path inside the archive
bool add_item_folder(mz_zip_archive *p_zip, const QString &in_path);

// adds file item and data to zip; 'fs_path' is the filesystem path; 'in_path' is the path inside the archive
bool add_item_file(mz_zip_archive *p_zip, const QString &fs_path, const QString &in_path);

// reads the file data and adds it to the archive
//bool addFileToZip(mz_zip_archive *p_zip, const QString &filePath, const QString &_path_in_zip);

// creates a list of the folder's contents and sends it to ::addItemsToZip()
bool addFolderToZip(mz_zip_archive *p_zip, const QString &folderPath);

// returns a list of folder content paths; addRoot: the root folder is added to the list
QStringList folderContent(const QString &folder, bool addRoot = true);
}

#endif // TOOLS_H
