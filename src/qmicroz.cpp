/*
 * This file is part of QMicroz,
 * licensed under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Author: Artem Vlasenko <artemvlas (at) proton (dot) me>
 * https://github.com/artemvlas
*/
#include "qmicroz.h"
#include "tools.h"
#include <QDir>
#include <QDebug>
#include <QStringBuilder>

QMicroz::QMicroz() {}

bool QMicroz::extract(const QString &zip_path)
{
    // extract to parent folder
    return extract(zip_path, QFileInfo(zip_path).absolutePath());
}

bool QMicroz::extract(const QString &zip_path, const QString &output_folder)
{
    qDebug() << "Extract:" << zip_path;
    qDebug() << "Output folder:" << output_folder;

    QDir _dir;

    // create output folder if it doesn't exist
    if (!QFileInfo::exists(output_folder) && !_dir.mkpath(output_folder)) {
        qWarning() << "Failed to create output folder:" << output_folder;
        return false;
    }

    // open zip archive
    mz_zip_archive __za;
    memset(&__za, 0, sizeof(__za));

    if (!mz_zip_reader_init_file(&__za, zip_path.toUtf8().constData(), 0)) {
        qWarning() << "Failed to open zip file:" << zip_path;
        return false;
    }

    // extracting...
    const mz_uint _num_items = mz_zip_reader_get_num_files(&__za);

    for (uint it = 0; it < _num_items; ++it) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&__za, it, &file_stat)) {
            qWarning() << "Failed to get file info:" << it;
            mz_zip_reader_end(&__za);
            return false;
        }

        qDebug() << "Extracting" << (it + 1) << '/' << _num_items << file_stat.m_filename;

        const QString _relpath = QString::fromUtf8(file_stat.m_filename);
        const QString _outpath = output_folder % tools::s_sep % _relpath;

        // create new path on the disk
        if (!_dir.mkpath(QFileInfo(_outpath).path())) {
            qWarning() << "Failed to create directory for file:" << _outpath;
            mz_zip_reader_end(&__za);
            return false;
        }

        // subfolder, no data to extract
        if (_relpath.endsWith(tools::s_sep))
            continue;

        // extract file
        if (!mz_zip_reader_extract_to_file(&__za, it, _outpath.toUtf8().constData(), 0)) {
            qWarning() << "Failed to extract file" << _relpath;
            mz_zip_reader_end(&__za);
            return false;
        }
    }

    // cleanup
    mz_zip_reader_end(&__za);
    qDebug() << "Unzip complete.";
    return true;
}

bool QMicroz::compress_(const QString &path)
{
    QFileInfo __fi(path);

    if (__fi.isFile())
        return compress_file(path);
    else if (__fi.isDir())
        return compress_folder(path);
    else {
        qDebug() << "QMicroz::compress | WRONG path:" << path;
        return false;
    }
}

bool QMicroz::compress_(const QStringList &paths)
{
    if (paths.isEmpty())
        return false;

    const QString _rootfolder = QFileInfo(paths.first()).absolutePath();
    const QString _zipname = QFileInfo(_rootfolder).fileName() + QStringLiteral(u".zip");
    const QString _zippath = _rootfolder % tools::s_sep % _zipname;

    return compress_list(paths, _zippath);
}

bool QMicroz::compress_file(const QString &source_path)
{
    const QString out_path = source_path + QStringLiteral(u".zip");
    return compress_file(source_path, out_path);
}

bool QMicroz::compress_file(const QString &source_path, const QString &zip_path)
{
    if (!QFileInfo::exists(source_path)) {
        qWarning() << "File not found:" << source_path;
        return false;
    }

    qDebug() << "Zipping file:" << source_path;
    qDebug() << "Output:" << zip_path;

    // TODO merge with tools::createArchive
    // create and open the output zip file
    mz_zip_archive __za;
    memset(&__za, 0, sizeof(__za));
    if (!mz_zip_writer_init_file(&__za, zip_path.toUtf8().constData(), 0)) {
        qWarning() << "Failed to create output file:" << zip_path;
        return false;
    }

    // process
    if (!tools::addFileToZip(&__za, source_path, QFileInfo(source_path).fileName())) {
        mz_zip_writer_end(&__za);
        return false;
    }

    // cleanup
    mz_zip_writer_finalize_archive(&__za);
    mz_zip_writer_end(&__za);
    qDebug() << "Done";
    return true;
}

bool QMicroz::compress_folder(const QString &source_path)
{
    QFileInfo __fi(source_path);
    const QString _file_name = __fi.fileName() + QStringLiteral(u".zip");
    const QString _out_path = __fi.absolutePath()
                              % tools::s_sep
                              % _file_name;

    return compress_folder(source_path, _out_path);
}

bool QMicroz::compress_folder(const QString &source_path, const QString &zip_path)
{
    if (!QFileInfo::exists(source_path)) {
        qWarning() << "Folder not found:" << source_path;
        return false;
    }

    qDebug() << "Zipping folder:" << source_path;
    qDebug() << "Output:" << zip_path;

    const QString _root = QFileInfo(source_path).absolutePath();
    return tools::createArchive(zip_path, tools::folderContent(source_path), _root);
}

bool QMicroz::compress_list(const QStringList &paths, const QString &zip_path)
{
    if (paths.isEmpty())
        return false;

    qDebug() << "Zipping:" << paths.size() << "items";
    qDebug() << "Output:" << zip_path;

    const QString _root = QFileInfo(paths.first()).absolutePath();

    // check if all paths are in the same root
    for (const QString &_path : paths) {
        if (_root != QFileInfo(_path).absolutePath()) {
            qDebug() << "ERROR: all items must be in the same folder!";
            return false;
        }
    }

    QStringList _worklist;

    // parsing the path list
    for (const QString &_path : paths) {
        QFileInfo __fi(_path);
        if (!__fi.exists() || __fi.isSymLink()) {
            qWarning() << "Skipped:" << _path;
            continue;
        }

        if (__fi.isFile())
            _worklist << _path;
        else
            _worklist << tools::folderContent(_path);
    }

    return tools::createArchive(zip_path, _worklist, _root);
}
