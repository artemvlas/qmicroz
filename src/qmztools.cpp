/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Copyright (c) 2024 Artem Vlasenko
*/
#include "qmztools.h"
#include <QDebug>
#include <QDirIterator>
#include <QStringBuilder>

namespace tools {
mz_zip_archive* za_new(const QString &zip_path, ZaType za_type)
{
    // create and open a zip archive
    mz_zip_archive *pZip = new mz_zip_archive();

    bool result = za_type ? mz_zip_writer_init_file(pZip, zip_path.toUtf8().constData(), 0)
                          : mz_zip_reader_init_file(pZip, zip_path.toUtf8().constData(), 0);

    if (!result) {
        qWarning() << "QMicroz: Failed to open zip file:" << zip_path;
        delete pZip;
        return nullptr;
    }

    return pZip;
}

mz_zip_archive_file_stat za_file_stat(void *pZip, int file_index)
{
    mz_zip_archive *p = static_cast<mz_zip_archive *>(pZip);

    mz_zip_archive_file_stat file_stat;
    if (mz_zip_reader_file_stat(p, file_index, &file_stat)) {
        return file_stat;
    }

    qWarning() << "QMicroz: Failed to get file info:" << file_index;
    return mz_zip_archive_file_stat();
}

bool za_close(mz_zip_archive *pZip)
{
    if (pZip && mz_zip_end(pZip)) {
        delete pZip;
        return true;
    }

    qWarning() << "QMicroz: Failed to close archive.";
    return false;
}

bool add_item_data(mz_zip_archive *pZip, const QString &item_path, const QByteArray &data)
{
    if (!mz_zip_writer_add_mem(pZip,
                               item_path.toUtf8().constData(),
                               data.constData(),
                               data.size(),
                               compressLevel(data.size())))
    {
        qWarning() << "QMicroz: Failed to compress file:" << item_path;
        return false;
    }

    return true;
}

bool add_item_data(mz_zip_archive *pZip, const QString &item_path, const QByteArray &data, const QDateTime &lastModified)
{
    time_t modified = lastModified.isValid() ? lastModified.toSecsSinceEpoch() : 0;

    return mz_zip_writer_add_mem_ex_v2(pZip,
                                       item_path.toUtf8().constData(),   // entry name
                                       data.constData(),                 // file data
                                       data.size(),                      // file size
                                       NULL, 0,
                                       compressLevel(data.size()),
                                       0, 0,
                                       modified > 0 ? &modified : NULL,  // last modified, NULL for current
                                       NULL, 0, NULL, 0);
}

bool add_item_folder(mz_zip_archive *pZip, const QString &item_path)
{
    return add_item_data(pZip,
                         toFolderName(item_path),
                         QByteArray());
}

bool add_item_file(mz_zip_archive *pZip, const QString &fs_path, const QString &item_path)
{
    return mz_zip_writer_add_file(pZip,                             // zip archive
                                  item_path.toUtf8().constData(),   // path inside the zip
                                  fs_path.toUtf8().constData(),     // filesystem path
                                  NULL, 0,
                                  compressLevel(QFileInfo(fs_path).size()));
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

    qWarning() << "QMicroz: Failed to extract file:" << file_index;
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

    qWarning() << "QMicroz: Failed to extract file:" << file_index;
    return QByteArray();
}

QStringList folderContent(const QString &folder)
{
    QStringList items;

    QDirIterator it(folder,
                    QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        items << it.next();
    }

    return items;
}

QMap<QString, QString> folderContentRel(const QString &folder)
{
    QMap<QString, QString> found;
    QDir dir(folder);

    QDirIterator it(folder,
                    QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden,
                    QDirIterator::Subdirectories);

    while (it.hasNext()) {
        const QString fullPath = it.next();
        found.insert(fullPath, dir.relativeFilePath(fullPath));
    }

    return found;
}

bool createFolder(const QString &path)
{
    if (QFileInfo::exists(path)
        || QDir().mkpath(path))
    {
        return true;
    }

    qWarning() << "QMicroz: Failed to create directory:" << path;
    return false;
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
