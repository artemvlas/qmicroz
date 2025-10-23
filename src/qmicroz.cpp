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

    qWarning() << "QMicroz: Wrong path:" << zip_path;
    return false;
}

bool QMicroz::setZipBuffer(const QByteArray &buffered_zip)
{
    if (!isArchive(buffered_zip)) {
        qWarning() << "QMicroz: The byte array is not zipped";
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

    qWarning() << "QMicroz: Failed to open buffered zip";
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
        qWarning() << "QMicroz: No output folder";

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
    if (!m_archive)
        return 0;

    return mz_zip_reader_get_num_files(static_cast<mz_zip_archive *>(m_archive));
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

    qDebug() << "QMicroz: Index not found:" << file_name;
    return -1;
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
        warningZipNotSet();
        return false;
    }

    return tools::extract_all_to_disk(static_cast<mz_zip_archive *>(m_archive),
                                      outputFolder(), m_verbose);
}

// !recreate_path >> place in the root of the output folder
bool QMicroz::extractIndex(int index, bool recreate_path)
{
    if (!m_archive) {
        warningZipNotSet();
        return false;
    }

    if (index == -1 || outputFolder().isEmpty())
        return false;

    // create output folder if it doesn't exist
    if (!tools::createFolder(outputFolder())) {
        qWarning() << "QMicroz: Failed to create folder:" << outputFolder();
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
        warningZipNotSet();
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
        warningZipNotSet();
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

void QMicroz::warningZipNotSet() const
{
    qWarning() << "QMicroz: Zip archive is not set.";
}

/*** STATIC functions ***/
bool QMicroz::extract(const QString &zip_path)
{
    // extract to parent folder
    return extract(zip_path, QFileInfo(zip_path).absolutePath());
}

bool QMicroz::extract(const QString &zip_path, const QString &output_folder)
{
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

/*** Compress ***/
bool QMicroz::compress(const QString &path)
{
    QFileInfo fi(path);

    if (!fi.isFile() && !fi.isDir()) {
        qWarning() << "QMicroz: Wrong path:" << path;
        return false;
    }

    const QString base_name = fi.isFile() ? fi.completeBaseName() : fi.fileName();
    const QString zip_name = base_name + s_zip_ext;
    const QString zip_path = tools::joinPath(fi.absolutePath(), zip_name);

    return compress(QStringList(path), zip_path);
}

bool QMicroz::compress(const QStringList &paths)
{
    if (paths.isEmpty())
        return false;

    const QString root_folder = QFileInfo(paths.first()).absolutePath();
    const QString zip_name = QFileInfo(root_folder).fileName() + s_zip_ext;
    const QString zip_path = tools::joinPath(root_folder, zip_name);

    return compress(paths, zip_path);
}

bool QMicroz::compress(const QString &source_path, const QString &zip_path)
{
    if (!QFileInfo::exists(source_path)) {
        qWarning() << "QMicroz: Wrong path:" << source_path;
        return false;
    }

    return compress(QStringList(source_path), zip_path);
}

bool QMicroz::compress(const QStringList &paths, const QString &zip_path)
{
    if (paths.isEmpty())
        return false;

    const QString root = QFileInfo(paths.first()).absolutePath();

    // check if all paths are in the same root
    if (paths.size() > 1) {
        for (const QString &path : paths) {
            if (root != QFileInfo(path).absolutePath()) {
                qWarning() << "QMicroz: All items must have the same parent folder.";
                return false;
            }
        }
    }

    QStringList worklist;

    // parsing the path list
    for (const QString &path : paths) {
        QFileInfo fi(path);
        if (!fi.exists() || fi.isSymLink()) {
            qWarning() << "QMicroz: Skipped:" << path;
            continue;
        }

        worklist << path;

        if (fi.isDir())
            worklist << tools::folderContent(path);
    }

    if (worklist.isEmpty()) {
        qWarning() << "QMicroz: No input paths.";
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
    }

    // cleanup
    tools::za_close(pZip);

    return res;
}

bool QMicroz::compress(const BufList &buf_data, const QString &zip_path)
{
    if (buf_data.isEmpty()) {
        qWarning() << "QMicroz: No input data.";
        return false;
    }

    // create and open the output zip file
    mz_zip_archive *pZip = tools::za_new(zip_path, tools::ZaWriter);

    if (!pZip)
        return false;

    // process
    BufList::const_iterator it;
    for (it = buf_data.constBegin(); it != buf_data.constEnd(); ++it) {
        if (!tools::add_item_data(pZip, it.key(), it.value())) {
            tools::za_close(pZip);
            return false;
        }
    }

    // success
    mz_zip_writer_finalize_archive(pZip);
    tools::za_close(pZip);

    return true;
}

bool QMicroz::compress(const QString &file_name,
                       const QByteArray &file_data,
                       const QString &zip_path)
{
    BufList buf;
    buf[file_name] = file_data;
    return compress(buf, zip_path);
}

/*** Additional ***/
bool QMicroz::isArchive(const QByteArray &data)
{
    return data.startsWith("PK");
}

bool QMicroz::isZipFile(const QString &filePath)
{
    QFile file(filePath);
    return file.open(QFile::ReadOnly) && isArchive(file.read(2));
}


/*** OBSOLETE ***/
bool QMicroz::compress_here(const QString &path)
{
    return compress(path);
}

bool QMicroz::compress_here(const QStringList &paths)
{
    return compress(paths);
}

bool QMicroz::compress_file(const QString &source_path)
{
    return compress(source_path);
}

bool QMicroz::compress_file(const QString &source_path, const QString &zip_path)
{
    return compress(source_path, zip_path);
}

bool QMicroz::compress_folder(const QString &source_path)
{
    return compress(source_path);
}

bool QMicroz::compress_folder(const QString &source_path, const QString &zip_path)
{
    return compress(source_path, zip_path);
}

bool QMicroz::compress_list(const QStringList &paths, const QString &zip_path)
{
    return compress(paths, zip_path);
}

bool QMicroz::compress_buf(const BufList &buf_data, const QString &zip_path)
{
    return compress(buf_data, zip_path);
}

bool QMicroz::compress_buf(const QByteArray &data, const QString &file_name, const QString &zip_path)
{
    return compress(file_name, data, zip_path);
}
