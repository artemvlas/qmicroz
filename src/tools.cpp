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
    if (item_paths.isEmpty()) {
        qDebug() << "No input paths. Nothing to zip.";
        return false;
    }

    // create and open the output zip file
    mz_zip_archive __za;
    memset(&__za, 0, sizeof(__za));
    if (!mz_zip_writer_init_file(&__za, zip_path.toUtf8().constData(), 0)) {
        qWarning() << "Failed to create output file:" << zip_path;
        return false;
    }

    // process
    if (!add_item_list(&__za, item_paths, zip_root))
        return false;

    // cleanup
    mz_zip_writer_finalize_archive(&__za);
    mz_zip_writer_end(&__za);
    qDebug() << "Done";
    return true;
}

bool add_item_data(mz_zip_archive *p_zip, const QString &_item_path, const QByteArray &_data)
{
    mz_uint _comp_level = (_data.size() > 40) ? MZ_DEFAULT_COMPRESSION : MZ_NO_COMPRESSION;

    qDebug() << "Adding:" << _item_path;

    if (!mz_zip_writer_add_mem(p_zip,
                               _item_path.toUtf8().constData(),
                               _data.constData(),
                               _data.size(),
                               _comp_level))
    {
        qWarning() << "Failed to compress file:" << _item_path;
        return false;
    }

    return true;
}

bool add_item_folder(mz_zip_archive *p_zip, const QString &in_path)
{
    return add_item_data(p_zip,
                         in_path.endsWith('/') ? in_path : in_path + '/',
                         QByteArray());
}

bool add_item_file(mz_zip_archive *p_zip, const QString &fs_path, const QString &in_path)
{
    mz_uint _comp_level = (QFileInfo(fs_path).size() > 40) ? MZ_DEFAULT_COMPRESSION : MZ_NO_COMPRESSION;

    qDebug() << "Adding:" << in_path;
    return mz_zip_writer_add_file(p_zip,                            // zip archive
                                  in_path.toUtf8().constData(),     // path inside the zip
                                  fs_path.toUtf8().constData(),     // filesystem path
                                  NULL, 0, _comp_level);
}

bool add_item_list(mz_zip_archive *p_zip, const QStringList &items, const QString &rootFolder)
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

mz_zip_archive* za_new(const QString &zip_path, ZaType za_type)
{
    // open zip archive
    mz_zip_archive *_za = new mz_zip_archive();
    //memset(&__za, 0, sizeof(__za));

    bool result = za_type ? mz_zip_writer_init_file(_za, zip_path.toUtf8().constData(), 0)
                          : mz_zip_reader_init_file(_za, zip_path.toUtf8().constData(), 0);

    if (!result) {
        qWarning() << "Failed to open zip file:" << zip_path;
        delete _za;
        return nullptr;
    }

    return _za;
}

mz_zip_archive_file_stat za_file_stat(mz_zip_archive* pZip, int file_index)
{
    mz_zip_archive_file_stat file_stat;
    if (!mz_zip_reader_file_stat(pZip, file_index, &file_stat)) {
        qWarning() << "Failed to get file info:" << file_index;
        //mz_zip_reader_end(&__za);
        return mz_zip_archive_file_stat();
    }

    return file_stat;
}

bool za_close(mz_zip_archive* pZip)
{
    if (pZip && mz_zip_end(pZip)) {
        delete pZip;
        qDebug() << "Archive closed";
        return true;
    }

    qWarning() << "Failed to close archive";
    return false;
}

QByteArray extract_data_to_buffer(mz_zip_archive* pZip, int file_index, bool copy_data)
{
    size_t __size = 0;
    char *_c = (char*)mz_zip_reader_extract_to_heap(pZip, file_index, &__size, 0);

    if (_c) {
        if (copy_data) {
            // COPY data to QByteArray
            QByteArray __b(_c, __size);

            // clear extracted from heap
            delete _c;

            return __b;
        }

        // Reference to the data in the QByteArray.
        // Data should be deleted on the caller side: delete _array.constData();
        return QByteArray::fromRawData(_c, __size);
    }

    qWarning() << "Failed to extract file:" << file_index;
    return QByteArray();
}

}// namespace tools
