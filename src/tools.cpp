/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Author: Artem Vlasenko
 * https://github.com/artemvlas
*/
#include "tools.h"
#include <QDebug>
#include <QDirIterator>
#include <QStringBuilder>

namespace tools {
bool createArchive(const QString &zip_path, const QStringList &item_paths, const QString &zip_root)
{
    // create and open the output zip file
    mz_zip_archive __za;
    memset(&__za, 0, sizeof(__za));
    if (!mz_zip_writer_init_file(&__za, zip_path.toUtf8().constData(), 0)) {
        qWarning() << "Failed to create output file:" << zip_path;
        return false;
    }

    // process
    if (!addItemsToZip(&__za, item_paths, zip_root))
        return false;

    // cleanup
    mz_zip_writer_finalize_archive(&__za);
    mz_zip_writer_end(&__za);
    qDebug() << "Done";
    return true;
}

bool addItemToZip(mz_zip_archive *p_zip, const QString &_item_path, const QByteArray &_data)
{
    qDebug() << "Adding:" << _item_path;

    if (!mz_zip_writer_add_mem(p_zip,
                               _item_path.toUtf8().constData(),
                               _data.constData(),
                               _data.size(),
                               MZ_DEFAULT_COMPRESSION))
    {
        qWarning() << "Failed to compress file:" << _item_path;
        return false;
    }

    return true;
}

bool addItemsToZip(mz_zip_archive *p_zip, const QStringList &items, const QString &rootFolder)
{
    QDir __d(rootFolder);

    // parsing a list of paths
    for (const QString &_item : items) {
        QFileInfo __fi(_item);
        const QString _relPath = __d.relativeFilePath(_item);

        // adding item
        if (__fi.isFile() && !add_item_file(p_zip, _item, _relPath)  // file
            || (__fi.isDir() && !add_item_folder(p_zip, _relPath)))  // subfolder
        {
            // adding failed
            mz_zip_writer_end(p_zip);
            return false;
        }
    }

    return true;
}

bool add_item_folder(mz_zip_archive *p_zip, const QString &in_path)
{
    return addItemToZip(p_zip,
                        in_path.endsWith('/') ? in_path : in_path + '/',
                        QByteArray());
}

bool add_item_file(mz_zip_archive *p_zip, const QString &fs_path, const QString &in_path)
{
    qDebug() << "Adding:" << in_path;
    return mz_zip_writer_add_file(p_zip,                            // zip archive
                                  in_path.toUtf8().constData(),     // path inside the zip
                                  fs_path.toUtf8().constData(),     // filesystem path
                                  NULL, 0, MZ_DEFAULT_COMPRESSION);
}

/*
bool addFileToZip(mz_zip_archive *p_zip, const QString &filePath, const QString &_path_in_zip)
{
    QFile _file(filePath);
    if (!_file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file:" << filePath;
        return false;
    }

    return addItemToZip(p_zip,            // zip archive
                        _path_in_zip,     // relative path
                        _file.readAll()); // file data
}*/

bool addFolderToZip(mz_zip_archive *p_zip, const QString &folderPath)
{
    const QStringList _items = folderContent(folderPath);
    return addItemsToZip(p_zip, _items, QFileInfo(folderPath).absolutePath());
}

QStringList folderContent(const QString &folder, bool addRoot)
{
    QStringList _items;

    if (addRoot) // add root folder
        _items << folder;

    QDirIterator it(folder,
                    QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Hidden,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        _items << it.next();
    }

    return _items;
}
}// namespace tools
