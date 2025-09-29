/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Copyright (c) 2024 Artem Vlasenko
*/
#include "tools.h"
#include <QDebug>
#include <QDirIterator>
#include <QStringBuilder>

namespace tools {
mz_zip_archive* za_new(const QString &zip_path, ZaType za_type)
{
    // create and open zip archive
    mz_zip_archive *pZip = new mz_zip_archive();

    bool result = za_type ? mz_zip_writer_init_file(pZip, zip_path.toUtf8().constData(), 0)
                          : mz_zip_reader_init_file(pZip, zip_path.toUtf8().constData(), 0);

    if (!result) {
        qWarning() << "Failed to open zip file:" << zip_path;
        delete pZip;
        return nullptr;
    }

    return pZip;
}

mz_zip_archive_file_stat za_file_stat(mz_zip_archive *pZip, int file_index)
{
    mz_zip_archive_file_stat file_stat;
    if (mz_zip_reader_file_stat(pZip, file_index, &file_stat)) {
        return file_stat;
    }

    qWarning() << "Failed to get file info:" << file_index;
    return mz_zip_archive_file_stat();
}

bool za_close(mz_zip_archive *pZip)
{
    if (pZip && mz_zip_end(pZip)) {
        delete pZip;
        return true;
    }

    qWarning() << "Failed to close archive";
    return false;
}

QString za_item_name(mz_zip_archive *pZip, int file_index)
{
    return za_file_stat(pZip, file_index).m_filename;
}

bool add_item_data(mz_zip_archive *pZip, const QString &item_path, const QByteArray &data)
{
    qDebug() << "Adding:" << item_path;

    if (!mz_zip_writer_add_mem(pZip,
                               item_path.toUtf8().constData(),
                               data.constData(),
                               data.size(),
                               compressLevel(data.size())))
    {
        qWarning() << "Failed to compress file:" << item_path;
        return false;
    }

    return true;
}

bool add_item_folder(mz_zip_archive *pZip, const QString &in_path)
{
    return add_item_data(pZip,
                         in_path.endsWith(s_sep) ? in_path : in_path + s_sep,
                         QByteArray());
}

bool add_item_file(mz_zip_archive *pZip, const QString &fs_path, const QString &in_path)
{
    qDebug() << "Adding:" << in_path;
    return mz_zip_writer_add_file(pZip,                             // zip archive
                                  in_path.toUtf8().constData(),     // path inside the zip
                                  fs_path.toUtf8().constData(),     // filesystem path
                                  NULL, 0,
                                  compressLevel(QFileInfo(fs_path).size()));
}

bool add_item_list(mz_zip_archive *pZip, const QStringList &items, const QString &rootFolder)
{
    QDir dir(rootFolder);

    // parsing a list of paths
    for (const QString &item : items) {
        QFileInfo fi(item);
        const QString relPath = dir.relativeFilePath(item);

        // adding item
        if ((fi.isFile() && !add_item_file(pZip, item, relPath))  // file
            || (fi.isDir() && !add_item_folder(pZip, relPath)))   // subfolder
        {
            // adding failed
            return false;
        }
    }

    return true;
}

bool extract_to_file(mz_zip_archive *pZip, int file_index, const QString &outpath)
{
    if (mz_zip_reader_extract_to_file(pZip,
                                      file_index,
                                      outpath.toUtf8().constData(),
                                      0))
    {
        return true;
    }

    qWarning() << "Failed to extract file:" << file_index;
    return false;
}

QByteArray extract_to_buffer(mz_zip_archive *pZip, int file_index, bool copy_data)
{
    size_t data_size = 0;
    char *ch_data = (char*)mz_zip_reader_extract_to_heap(pZip, file_index, &data_size, 0);

    if (ch_data) {
        if (copy_data) {
            // COPY data to QByteArray
            QByteArray __ba(ch_data, data_size);

            // clear extracted from heap
            delete ch_data;

            return __ba;
        }

        // Reference to the data in the QByteArray.
        // Data should be deleted on the caller side: delete _array.constData();
        return QByteArray::fromRawData(ch_data, data_size);
    }

    qWarning() << "Failed to extract file:" << file_index;
    return QByteArray();
}

bool extract_all_to_disk(mz_zip_archive *pZip, const QString &output_folder, bool verbose)
{
    if (output_folder.isEmpty()) {
        qWarning() << "No output folder provided";
        return false;
    }

    const int num_items = mz_zip_reader_get_num_files(pZip);

    if (num_items == 0) {
        qWarning() << "No files to extract";
        return false;
    }

    if (verbose)
        qDebug() << "Extracting" << num_items << "items to:" << output_folder;

    // extracting...
    bool is_success = true;
    for (int it = 0; it < num_items; ++it) {
        const QString filename = za_item_name(pZip, it);

        if (verbose)
            qDebug() << "Extracting:" << (it + 1) << '/' << num_items << filename;

        const QString outpath = joinPath(output_folder, filename);

        // create new path on the disk
        const QString parent_folder = QFileInfo(outpath).absolutePath();
        if (!createFolder(parent_folder)) {
            is_success = false;
            break;
        }

        // subfolder, no data to extract
        if (filename.endsWith(s_sep))
            continue;

        // extract file
        if (!extract_to_file(pZip, it, outpath)) {
            is_success = false;
            break;
        }
    }

    if (verbose)
        qDebug() << (is_success ? "Unzip complete." : "Unzip failed.");

    return is_success;
}

QStringList folderContent(const QString &folder, bool addRoot)
{
    QStringList items;

    if (addRoot) // add root folder
        items << folder;

    QDirIterator it(folder,
                    QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Hidden,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        items << it.next();
    }

    return items;
}

bool createFolder(const QString &path)
{
    if (QFileInfo::exists(path)
        || QDir().mkpath(path))
    {
        return true;
    }

    qWarning() << "Failed to create directory:" << path;
    return false;
}

bool endsWithSlash(const QString &path)
{
    return (path.endsWith('/') || path.endsWith('\\'));
}

QString joinPath(const QString &abs_path, const QString &rel_path)
{
    return endsWithSlash(abs_path) ? abs_path + rel_path
                                   : abs_path % s_sep % rel_path;
}

mz_uint compressLevel(qint64 data_size)
{
    return (data_size > 40) ? MZ_DEFAULT_COMPRESSION : MZ_NO_COMPRESSION;
}

} // namespace tools
