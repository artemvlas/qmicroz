/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Copyright (c) 2024 Artem Vlasenko
*/
#include "qmicroz.h"
#include "tools.h"
#include <QDir>
#include <QDebug>

const QString QMicroz::s_zip_ext = QStringLiteral(u".zip");

QMicroz::QMicroz() {}

QMicroz::QMicroz(const char* zip_path)
{
    setZipFile(QString(zip_path));
}

QMicroz::QMicroz(const QString &zip_path)
{
    setZipFile(zip_path);
}

QMicroz::QMicroz(const QByteArray &buffered_zip)
{
    setZipBuffer(buffered_zip);
}

QMicroz::~QMicroz()
{
    closeArchive();
}

void QMicroz::setVerbose(bool enable)
{
    m_verbose = enable;
}

bool QMicroz::setZipFile(const QString &zip_path)
{
    if (isZipFile(zip_path)) {
        // try to open zip archive
        mz_zip_archive *pZip = tools::za_new(zip_path, tools::ZaReader);

        if (pZip) {
            // close the currently opened one if any
            closeArchive();

            m_archive = pZip;
            m_zip_path = zip_path;
            updateZipContents();
            setOutputFolder(); // zip file's parent folder
            return true;
        }
    }

    qWarning() << "Wrong path:" << zip_path;
    return false;
}

bool QMicroz::setZipBuffer(const QByteArray &buffered_zip)
{
    if (!isArchive(buffered_zip)) {
        qWarning() << "The byte array is not zipped";
        return false;
    }

    // open zip archive
    mz_zip_archive *pZip = new mz_zip_archive();

    if (mz_zip_reader_init_mem(pZip, buffered_zip.constData(), buffered_zip.size(), 0)) {
        // close the currently opened one if any
        closeArchive();

        // set the new one
        m_archive = pZip;
        updateZipContents();
        return true;
    }

    qWarning() << "Failed to open buffered zip";
    delete pZip;
    return false;
}

void QMicroz::setOutputFolder(const QString &output_folder)
{
    if (output_folder.isEmpty() && !m_zip_path.isEmpty()) {
        // set zip file's parent folder
        m_output_folder = QFileInfo(m_zip_path).absolutePath();
        return;
    }

    m_output_folder = output_folder;
}

const QString& QMicroz::outputFolder() const
{
    if (m_output_folder.isEmpty())
        qWarning() << "No output folder setted";

    return m_output_folder;
}

void QMicroz::closeArchive()
{
    if (!m_archive)
        return;

    tools::za_close(static_cast<mz_zip_archive *>(m_archive));
    m_archive = nullptr;
    m_zip_contents.clear();
    m_zip_path.clear();
    m_output_folder.clear();
}

const ZipContents& QMicroz::updateZipContents()
{
    m_zip_contents.clear();

    if (!m_archive)
        return m_zip_contents;

    mz_zip_archive *pZip = static_cast<mz_zip_archive *>(m_archive);

    // iterating...
    for (int it = 0; it < count(); ++it) {
        const QString filename = tools::za_item_name(pZip, it);
        if (filename.isEmpty())
            break;

        m_zip_contents[it] = filename;
    }

    return m_zip_contents;
}

qint64 QMicroz::sizeUncompressed() const
{
    qint64 total_size = 0;

    for (int it = 0; it < count(); ++it) {
        total_size += sizeUncompressed(it);
    }

    return total_size;
}

const QString& QMicroz::zipFilePath() const
{
    return m_zip_path;
}

const ZipContents& QMicroz::contents() const
{
    return m_zip_contents;
}

int QMicroz::count() const
{
    return mz_zip_reader_get_num_files(static_cast<mz_zip_archive *>(m_archive));
}

bool QMicroz::isFolder(int index) const
{
    return name(index).endsWith(tools::s_sep); // '/'
}

bool QMicroz::isFile(int index) const
{
    const QString item_name = name(index);
    return !item_name.isEmpty() && !item_name.endsWith(tools::s_sep);
}

QString QMicroz::name(int index) const
{
    return contents().value(index);
}

qint64 QMicroz::sizeCompressed(int index) const
{
    if (!m_archive)
        return 0;

    return tools::za_file_stat(static_cast<mz_zip_archive *>(m_archive), index).m_comp_size;
}

qint64 QMicroz::sizeUncompressed(int index) const
{
    if (!m_archive)
        return 0;

    return tools::za_file_stat(static_cast<mz_zip_archive *>(m_archive), index).m_uncomp_size;
}

QDateTime QMicroz::lastModified(int index) const
{
    if (!m_archive)
        return QDateTime();

    const qint64 sec = tools::za_file_stat(static_cast<mz_zip_archive *>(m_archive), index).m_time;

    return sec > 0 ? QDateTime::fromSecsSinceEpoch(sec) : QDateTime();
}

bool QMicroz::extractAll()
{
    if (!m_archive) {
        qWarning() << "No zip archive setted";
        return false;
    }

    return tools::extract_all_to_disk(static_cast<mz_zip_archive *>(m_archive),
                                      outputFolder(), m_verbose);
}

// !recreate_path >> place in the root of the output folder
bool QMicroz::extractIndex(int index, bool recreate_path)
{
    if (!m_archive) {
        qWarning() << "No zip archive setted";
        return false;
    }

    if (index == -1 || outputFolder().isEmpty())
        return false;

    if (m_verbose) {
        qDebug() << "Extract:" << index << "from:" << m_zip_path;
        qDebug() << "Output folder:" << outputFolder();
    }

    // create output folder if it doesn't exist
    if (!tools::createFolder(outputFolder())) {
        return false;
    }

    // extracting...
    // the name is also a relative path inside the archive
    const QString filename = name(index);
    if (filename.isEmpty())
        return false;

    if (m_verbose)
        qDebug() << "Extracting:" << filename;

    const QString outpath = tools::joinPath(outputFolder(),
                                            recreate_path ? filename : QFileInfo(filename).fileName());

    // create a new path on disk
    const QString parent_folder = QFileInfo(outpath).absolutePath();
    if (!tools::createFolder(parent_folder)) {
        return false;
    }

    // subfolder, no data to extract
    if (filename.endsWith(tools::s_sep)) {
        if (m_verbose)
            qDebug() << "Subfolder extracted";
    }
    // extract file
    else if (!tools::extract_to_file(static_cast<mz_zip_archive *>(m_archive), index, outpath)) {
        return false;
    }

    if (m_verbose)
        qDebug() << "Unzip complete.";

    return true;
}

bool QMicroz::extractFile(const QString &file_name, bool recreate_path)
{
    return extractIndex(findIndex(file_name), recreate_path);
}

BufList QMicroz::extractToBuf() const
{
    BufList res;

    if (!m_archive) {
        qWarning() << "No archive setted";
        return res;
    }

    if (m_verbose)
        qDebug() << "Extracting to RAM:" << (m_zip_path.isEmpty() ? "buffered zip" : m_zip_path);

    // extracting...
    for (int it = 0; it < count(); ++it) {
        const QString filename = name(it);
        if (filename.isEmpty())
            break;

        // subfolder, no data to extract
        if (filename.endsWith(tools::s_sep))
            continue;

        if (m_verbose)
            qDebug() << "Extracting:" << (it + 1) << '/' << count() << filename;

        // extract file
        const QByteArray data = extractData(it);
        if (!data.isNull())
            res[filename] = data;
    }

    if (m_verbose)
        qDebug() << "Unzipped:" << res.size() << "files";

    return res;
}

BufFile QMicroz::extractToBuf(int index) const
{
    BufFile res;

    if (!m_archive) {
        qWarning() << "No archive setted";
        return res;
    }

    if (index == -1)
        return res;

    if (m_verbose)
        qDebug() << "Extracting to RAM:" << (m_zip_path.isEmpty() ? "buffered zip" : m_zip_path);

    const QString filename = name(index);
    if (filename.isEmpty())
        return res;

    // subfolder, no data to extract
    if (filename.endsWith(tools::s_sep)) {
        if (m_verbose)
            qDebug() << "Subfolder, no data to extract:" << filename;
        return res;
    }

    if (m_verbose)
        qDebug() << "Extracting:" << filename;

    // extract file
    const QByteArray ex_data = extractData(index);

    if (!ex_data.isNull()) {
        res.name = filename;
        res.data = ex_data;
        res.modified = lastModified(index);

        if (m_verbose)
            qDebug() << "Unzipped:" << ex_data.size() << "bytes";
    }

    return res;
}

BufFile QMicroz::extractFileToBuf(const QString &file_name) const
{
    return extractToBuf(findIndex(file_name));
}

// Recommended in most cases if speed and memory requirements are not critical.
QByteArray QMicroz::extractData(int index) const
{
    return tools::extract_to_buffer(static_cast<mz_zip_archive *>(m_archive), index);
}

// This function is faster and consumes less resources than the previous one,
// but requires an additional delete operation to avoid memory leaks. ( delete _array.constData(); )
QByteArray QMicroz::extractDataRef(int index) const
{
    return tools::extract_to_buffer(static_cast<mz_zip_archive *>(m_archive),
                                    index, false);
}

// STATIC functions ---->>>
bool QMicroz::extract(const QString &zip_path)
{
    // extract to parent folder
    return extract(zip_path, QFileInfo(zip_path).absolutePath());
}

bool QMicroz::extract(const QString &zip_path, const QString &output_folder)
{
    //qDebug() << "Extract:" << zip_path;
    //qDebug() << "Output folder:" << output_folder;

    // open zip archive
    mz_zip_archive *pZip = tools::za_new(zip_path, tools::ZaReader);

    if (!pZip)
        return false;

    // extracting...
    bool is_success = tools::extract_all_to_disk(pZip, output_folder);

    // finish
    tools::za_close(pZip);
    return is_success;
}

bool QMicroz::compress_here(const QString &path)
{
    QFileInfo fi(path);

    if (fi.isFile()) {
        return compress_file(path);
    } else if (fi.isDir()) {
        return compress_folder(path);
    } else {
        qDebug() << "QMicroz::compress | WRONG path:" << path;
        return false;
    }
}

bool QMicroz::compress_here(const QStringList &paths)
{
    if (paths.isEmpty())
        return false;

    const QString rootfolder = QFileInfo(paths.first()).absolutePath();
    const QString zipname = QFileInfo(rootfolder).fileName() + s_zip_ext;
    const QString zippath = tools::joinPath(rootfolder, zipname);

    return compress_list(paths, zippath);
}

bool QMicroz::compress_file(const QString &source_path)
{
    QFileInfo fi(source_path);
    const QString zip_name = fi.completeBaseName() + s_zip_ext;
    const QString out_path = tools::joinPath(fi.absolutePath(), zip_name);

    return compress_file(source_path, out_path);
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
    QFileInfo fi(source_path);
    const QString file_name = fi.fileName() + s_zip_ext;
    const QString parent_folder = fi.absolutePath();
    const QString out_path = tools::joinPath(parent_folder, file_name);

    return compress_folder(source_path, out_path);
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

    const QString root = QFileInfo(paths.first()).absolutePath();
    const QString info = paths.size() == 1 ? paths.first()
                                           : (QString::number(paths.size()) + QStringLiteral(u" items"));

    qDebug() << "Zipping:" << info;
    qDebug() << "Output:" << zip_path;

    // check if all paths are in the same root
    if (paths.size() > 1) {
        for (const QString &path : paths) {
            if (root != QFileInfo(path).absolutePath()) {
                qWarning() << "ERROR: all items must be in the same folder!";
                return false;
            }
        }
    }

    QStringList worklist;

    // parsing the path list
    for (const QString &path : paths) {
        QFileInfo fi(path);
        if (!fi.exists() || fi.isSymLink()) {
            qWarning() << "Skipped:" << path;
            continue;
        }

        if (fi.isFile())
            worklist << path;
        else
            worklist << tools::folderContent(path);
    }

    if (worklist.isEmpty()) {
        qWarning() << "No input paths. Nothing to zip.";
        return false;
    }

    // create and open the output zip file
    mz_zip_archive *pZip = tools::za_new(zip_path, tools::ZaWriter);

    if (!pZip)
        return false;

    // process
    const bool res = tools::add_item_list(pZip, worklist, root);

    if (res) {
        mz_zip_writer_finalize_archive(pZip);
        qDebug() << worklist.size() << "items zipped";
    }

    // cleanup
    tools::za_close(pZip);

    return res;
}

bool QMicroz::compress_buf(const BufList &buf_data, const QString &zip_path)
{
    qDebug() << "Zipping buffered data to:" << zip_path;

    if (buf_data.isEmpty()) {
        qWarning() << "No input data. Nothing to zip.";
        return false;
    }

    // create and open the output zip file
    mz_zip_archive *pZip = tools::za_new(zip_path, tools::ZaWriter);

    if (!pZip)
        return false;

    // process
    BufList::const_iterator it;
    for (it = buf_data.constBegin(); it != buf_data.constEnd(); ++it) {
        qDebug() << "Adding:" << it.key();

        if (!tools::add_item_data(pZip, it.key(), it.value())) {
            tools::za_close(pZip);
            return false;
        }
    }

    // cleanup
    mz_zip_writer_finalize_archive(pZip);
    tools::za_close(pZip);
    qDebug() << "Done";
    return true;
}

bool QMicroz::compress_buf(const QByteArray &data, const QString &file_name, const QString &zip_path)
{
    BufList buf;
    buf[file_name] = data;
    return compress_buf(buf, zip_path);
}

int QMicroz::findIndex(const QString &file_name) const
{
    ZipContents::const_iterator it;

    // full path matching
    for (it = m_zip_contents.constBegin(); it != m_zip_contents.constEnd(); ++it) {
        if (file_name == it.value())
            return it.key();
    }

    // deep search, matching only the name
    if (!file_name.contains(tools::s_sep)) {
        for (it = m_zip_contents.constBegin(); it != m_zip_contents.constEnd(); ++it) {
            if (!it.value().endsWith('/') // if not a subfolder
                && file_name == QFileInfo(it.value()).fileName())
            {
                return it.key();
            }
        }
    }

    qDebug() << "Index not found:" << file_name;
    return -1;
}

bool QMicroz::isArchive(const QByteArray &data)
{
    return data.startsWith("PK");
}

bool QMicroz::isZipFile(const QString &filePath)
{
    QFile file(filePath);
    return file.open(QFile::ReadOnly) && isArchive(file.read(2));
}
