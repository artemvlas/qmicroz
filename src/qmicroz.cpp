/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Author: Artem Vlasenko
 * https://github.com/artemvlas
*/
#include "qmicroz.h"
#include "tools.h"
#include <QDir>
#include <QDebug>
#include <QStringBuilder>

QMicroz::QMicroz() {}

QMicroz::QMicroz(const QString &zip_path)
{
    setZipFile(zip_path);
}

bool QMicroz::setZipFile(const QString &zip_path)
{
    if (QFileInfo(zip_path).isFile()) {
        // try to open zip archive
        mz_zip_archive *_za = tools::za_new(zip_path, tools::ZaReader);

        if (_za) {
            closeArchive();

            m_archive = _za;
            m_zip_path = zip_path;
            updateZipContents();
            return true;
        }
    }

    qWarning() << "Wrong path:" << zip_path;
    return false;
}

QMicroz::~QMicroz()
{
    closeArchive();
}

bool QMicroz::closeArchive()
{
    if (m_archive) {
        mz_zip_archive *_za = static_cast<mz_zip_archive *>(m_archive);
        tools::za_close(_za);
        m_archive = nullptr;
        return true;
    }

    return false;
}

const ZipContentsList& QMicroz::updateZipContents()
{
    m_zip_contents.clear();

    if (!m_archive)
        return m_zip_contents;

    mz_zip_archive *_za = static_cast<mz_zip_archive *>(m_archive);

    // iterating...
    const mz_uint _num_items = mz_zip_reader_get_num_files(_za);

    for (uint it = 0; it < _num_items; ++it) {
        const QString _filename = tools::za_file_stat(_za, it).m_filename;
        if (_filename.isEmpty()) {
            break;
        }

        m_zip_contents[it] = _filename;
    }

    return m_zip_contents;
}

const ZipContentsList& QMicroz::zipContents()
{
    if (m_zip_contents.isEmpty())
        updateZipContents();

    return m_zip_contents;
}

BufFileList QMicroz::extract_to_ram() const
{
    qDebug() << "Extracting to ram:" << m_zip_path;

    BufFileList _res;

    if (!m_archive) {
        qWarning() << "No archive setted";
        return _res;
    }

    mz_zip_archive *_za = static_cast<mz_zip_archive *>(m_archive);

    // extracting...
    const mz_uint _num_items = mz_zip_reader_get_num_files(_za);

    for (uint it = 0; it < _num_items; ++it) {
        const QString _filename = tools::za_item_name(_za, it);
        if (_filename.isEmpty()) {
            break;
        }

        // subfolder, no data to extract
        if (_filename.endsWith('/'))
            continue;

        qDebug() << "Extracting" << (it + 1) << '/' << _num_items << _filename;

        // extract file
        const QByteArray _data = tools::extract_to_buffer(_za, it);
        if (!_data.isNull())
            _res[_filename] = _data;
    }

    qDebug() << "Unzipped:" << _res.size() << "files";
    return _res;
}

BufFile QMicroz::extract_to_ram(const QString &file_name)
{
    return extract_to_ram(findIndex(file_name));
}

BufFile QMicroz::extract_to_ram(const int file_index) const
{
    if (file_index == -1)
        return BufFile();

    qDebug() << "Extracting to ram buffer:" << m_zip_path;

    BufFile _res;

    if (!m_archive) {
        qWarning() << "No archive setted";
        return _res;
    }

    mz_zip_archive *_za = static_cast<mz_zip_archive *>(m_archive);

    const QString _filename = tools::za_item_name(_za, file_index);
    if (_filename.isEmpty())
        return _res;

    // subfolder, no data to extract
    if (_filename.endsWith('/')) {
        qDebug() << "Subfolder, no data to extract:" << _filename;
        return _res;
    }

    qDebug() << "Extracting:" << _filename;

    // extract file
    const QByteArray _data = tools::extract_to_buffer(_za, file_index);

    if (!_data.isNull()) {
        _res.m_name = _filename;
        _res.m_data = _data;

        qDebug() << "Unzipped:" << _data.size() << "bytes";
    }

    return _res;
}

bool QMicroz::extractAll(const QString &output_folder) const
{
    return extract(m_zip_path, output_folder);
}

bool QMicroz::extractFile(const QString &file_path, const QString &output_folder)
{
    return extractFile(findIndex(file_path), output_folder);
}

bool QMicroz::extractFile(const int file_index, const QString &output_folder) const
{
    if (file_index == -1)
        return false;

    qDebug() << "Extract:" << m_zip_path;
    qDebug() << "Output folder:" << output_folder;

    if (!m_archive) {
        qDebug() << "No archive setted";
        return false;
    }

    // create output folder if it doesn't exist
    if (!tools::createFolder(output_folder)) {
        return false;
    }

    mz_zip_archive *_za = static_cast<mz_zip_archive *>(m_archive);

    // extracting...
    const QString _filename = tools::za_item_name(_za, file_index);
    if (_filename.isEmpty()) {
        return false;
    }

    qDebug() << "Extracting:" << _filename;
    const bool is_out_ends_slash = (output_folder.endsWith('/') || output_folder.endsWith('\\'));
    const QString _outpath = is_out_ends_slash ? output_folder + _filename
                                               : output_folder % tools::s_sep % _filename;

    // create new path on the disk
    const QString _parent_folder = QFileInfo(_outpath).absolutePath();
    if (!tools::createFolder(_parent_folder)) {
        return false;
    }

    // subfolder, no data to extract
    if (_filename.endsWith(tools::s_sep)) {
        qDebug() << "Subfolder extracted";
    }
    // extract file
    else if (!tools::extract_to_file(_za, file_index, _outpath)) {
        return false;
    }

    qDebug() << "Unzip complete.";
    return true;
}

// STATIC functions ---->>>
bool QMicroz::extract(const QString &zip_path)
{
    // extract to parent folder
    return extract(zip_path, QFileInfo(zip_path).absolutePath());
}

bool QMicroz::extract(const QString &zip_path, const QString &output_folder)
{
    qDebug() << "Extract:" << zip_path;
    qDebug() << "Output folder:" << output_folder;

    // create output folder if it doesn't exist
    if (!tools::createFolder(output_folder)) {
        return false;
    }

    // open zip archive
    mz_zip_archive *__za = tools::za_new(zip_path, tools::ZaReader);
    if (!__za) {
        return false;
    }

    // extracting...
    bool is_success = true;
    const bool is_out_ends_slash = (output_folder.endsWith('/') || output_folder.endsWith('\\'));
    const mz_uint _num_items = mz_zip_reader_get_num_files(__za);

    for (uint it = 0; it < _num_items; ++it) {
        const QString _filename = tools::za_item_name(__za, it);

        qDebug() << "Extracting" << (it + 1) << '/' << _num_items << _filename;

        const QString _outpath = is_out_ends_slash ? output_folder + _filename
                                                   : output_folder % tools::s_sep % _filename;

        // create new path on the disk
        const QString _parent_folder = QFileInfo(_outpath).absolutePath();
        if (!tools::createFolder(_parent_folder)) {
            is_success = false;
            break;
        }

        // subfolder, no data to extract
        if (_filename.endsWith(tools::s_sep))
            continue;

        // extract file
        if (!tools::extract_to_file(__za, it, _outpath)) {
            is_success = false;
            break;
        }
    }

    // result
    tools::za_close(__za);
    qDebug() << (is_success ? "Unzip complete." : "Unzip failed.");
    return is_success;
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
    QFileInfo __fi(source_path);
    const QString _zip_name = __fi.completeBaseName() + QStringLiteral(u".zip");
    const QString _out_path = __fi.absolutePath() % tools::s_sep % _zip_name;

    return compress_file(source_path, _out_path);
}

bool QMicroz::compress_file(const QString &source_path, const QString &zip_path)
{
    if (!QFileInfo::exists(source_path)) {
        qWarning() << "File not found:" << source_path;
        return false;
    }

    return compress_list({ source_path }, zip_path);
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

    return compress_list({ source_path }, zip_path);
}

bool QMicroz::compress_list(const QStringList &paths, const QString &zip_path)
{
    if (paths.isEmpty())
        return false;

    const QString _root = QFileInfo(paths.first()).absolutePath();
    const QString _info = QStringLiteral(u"Zipping: ")
                          + (paths.size() == 1 ? paths.first() : (QString::number(paths.size()) + QStringLiteral(u" items")));
    qDebug() << _info;
    qDebug() << "Output:" << zip_path;

    // check if all paths are in the same root
    if (paths.size() > 1) {
        for (const QString &_path : paths) {
            if (_root != QFileInfo(_path).absolutePath()) {
                qDebug() << "ERROR: all items must be in the same folder!";
                return false;
            }
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

bool QMicroz::compress_buf(const BufFileList &buf_data, const QString &zip_path)
{
    qDebug() << "Zipping buffered data to:" << zip_path;

    if (buf_data.isEmpty()) {
        qDebug() << "No input data. Nothing to zip.";
        return false;
    }

    // create and open the output zip file
    mz_zip_archive *__za = tools::za_new(zip_path, tools::ZaWriter);
    if (!__za) {
        return false;
    }

    // process
    BufFileList::const_iterator it;
    for (it = buf_data.constBegin(); it != buf_data.constEnd(); ++it) {
        if (!tools::add_item_data(__za, it.key(), it.value())) {
            tools::za_close(__za);
            return false;
        }
    }

    // cleanup
    mz_zip_writer_finalize_archive(__za);
    tools::za_close(__za);
    qDebug() << "Done";
    return true;
}

bool QMicroz::compress_buf(const QByteArray &data, const QString &file_name, const QString &zip_path)
{
    BufFileList _buf;
    _buf[file_name] = data;
    return compress_buf(_buf, zip_path);
}

int QMicroz::findIndex(const QString &file_name)
{
    if (m_zip_contents.isEmpty())
        updateZipContents();

    ZipContentsList::const_iterator it;
    for (it = m_zip_contents.constBegin(); it != m_zip_contents.constEnd(); ++it) {
        if (file_name == it.value())
            return it.key();
    }

    qDebug() << "Index not found:" << file_name;
    return -1;
}
