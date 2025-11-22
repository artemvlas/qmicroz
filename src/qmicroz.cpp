/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Copyright (c) 2024 Artem Vlasenko
*/

#define WARNING_ZIPNOTSET "QMicroz: Zip archive is not set."
#define WARNING_WRONGMODE "QMicroz: Wrong archive mode."
#define WARNING_WRONGPATH "QMicroz: Wrong path:"
#define WARNING_NOINPUTDATA "QMicroz: No input data."

#include "qmicroz.h"
#include "qmztools.h"
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

bool QMicroz::isModeReading() const
{
    return tools::zipMode(m_archive) == mz_zip_mode::MZ_ZIP_MODE_READING;
}

bool QMicroz::isModeWriting() const
{
    return tools::zipMode(m_archive) == mz_zip_mode::MZ_ZIP_MODE_WRITING;
}

void QMicroz::setVerbose(bool enable)
{
    m_verbose = enable;
}

bool QMicroz::setZipFile(const QString &zip_path)
{
    tools::ZaType type = isZipFile(zip_path) ? tools::ZaReader : tools::ZaWriter;

    if (type == tools::ZaWriter && QFileInfo::exists(zip_path))
        return false;

    // try to open zip archive
    mz_zip_archive *pZip = tools::za_new(zip_path, type);

    if (pZip) {
        // close the currently opened one if any
        closeArchive();

        m_archive = pZip;
        m_zip_path = zip_path;

        if (isModeReading()) {
            updateZipContents();
            setOutputFolder(); // zip file's parent folder
        }

        return true;
    }

    qWarning() << WARNING_WRONGPATH << zip_path;
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

    mz_zip_archive *pZip = static_cast<mz_zip_archive *>(m_archive);

    if (isModeWriting())
        mz_zip_writer_finalize_archive(pZip);

    tools::za_close(pZip);
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
            if (tools::isFileItem(it.value())
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
    return tools::isFolderItem(name(index));
}

bool QMicroz::isFile(int index) const
{
    return tools::isFileItem(name(index));
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

bool QMicroz::addToZip(const QString &source_path)
{
    if (!QFileInfo::exists(source_path))
        return false;

    return addToZip(source_path, QFileInfo(source_path).fileName());
}

bool QMicroz::addToZip(const QString &source_path, const QString &entry_path)
{
    if (!m_archive || entry_path.isEmpty() || !QFileInfo::exists(source_path))
        return false;

    if (!isModeWriting()) {
        qWarning() << WARNING_WRONGMODE;
        return false;
    }

    QFileInfo fi_source(source_path);
    mz_zip_archive *pZip = static_cast<mz_zip_archive *>(m_archive);

    if (fi_source.isFile()) {
        bool res = tools::add_item_file(pZip, source_path, entry_path);
        if (res)
            m_zip_contents[m_zip_contents.size()] = entry_path;

        return res;
    } else if (fi_source.isDir()) {
        QMap<QString, QString> folderContent;
        folderContent.insert(source_path, QString());
        folderContent.insert(tools::folderContentRel(source_path));

        // parsing a list of paths
        QMap<QString, QString>::const_iterator it;
        for (it = folderContent.constBegin(); it != folderContent.constEnd(); ++it) {
            QFileInfo fi(it.key());
            const QString relPath = tools::joinPath(entry_path, it.value());

            if (m_verbose)
                qDebug() << "Adding:" << relPath;

            // adding item
            if (fi.isFile() && tools::add_item_file(pZip, it.key(), relPath)) // file
                m_zip_contents[m_zip_contents.size()] = relPath;
            else if (fi.isDir() && tools::add_item_folder(pZip, relPath))     // subfolder
                m_zip_contents[m_zip_contents.size()] = relPath + '/';
            else
                return false; // adding failed
        }

        return true;
    }

    return false;
}

bool QMicroz::addToZip(const BufFile &buf_file)
{
    if (!isModeWriting()) {
        qWarning() << WARNING_WRONGMODE;
        return false;
    }

    if (!buf_file)
        return false;

    mz_zip_archive *pZip = static_cast<mz_zip_archive *>(m_archive);

    bool res = buf_file.name.endsWith('/') ? tools::add_item_folder(pZip, buf_file.name)
                                           : tools::add_item_data(pZip, buf_file.name, buf_file.data, buf_file.modified);

    if (res)
        m_zip_contents[m_zip_contents.size()] = buf_file.name;

    return res;
}

bool QMicroz::addToZip(const BufList &buf_data)
{
    BufList::const_iterator it;
    for (it = buf_data.constBegin(); it != buf_data.constEnd(); ++it) {
        if (addToZip(BufFile(it.key(), it.value())))
            m_zip_contents[m_zip_contents.size()] = it.key();
        else
            return false;
    }

    return true;
}

bool QMicroz::extractAll()
{
    if (count() == 0) {
        qWarning() << "QMicroz: No files to extract.";
        return false;
    }

    if (m_verbose)
        qDebug() << "Extracting" << count() << "items to:" << outputFolder();

    for (int i = 0; i < count(); ++i) {
        if (!extractIndex(i)) {
            qWarning() << "Extracting failed:" << i << name(i);
            return false;
        }
    }

    if (m_verbose)
        qDebug() << "Complete.";

    return true;
}

bool QMicroz::extractIndex(int index, bool recreate_path)
{
    if (index == -1)
        return false;

    if (!m_archive) {
        qWarning() << WARNING_ZIPNOTSET;
        return false;
    }

    if (!isModeReading()) {
        qWarning() << WARNING_WRONGMODE;
        return false;
    }

    if (outputFolder().isEmpty()) {
        qWarning() << "QMicroz: No output folder.";
        return false;
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
        qDebug() << "Extracting:" << filename; // "Extracting:" << (index + 1) << '/' << count() << filename;

    // <!recreate_path> to place in the root of the output folder
    const QString outpath = tools::joinPath(outputFolder(),
                                            recreate_path ? filename : QFileInfo(filename).fileName());

    // create a new path on disk
    const QString parent_folder = QFileInfo(outpath).absolutePath();
    if (!tools::createFolder(parent_folder)) {
        return false;
    }

    // subfolder, no data to extract
    if (tools::isFolderItem(filename)) {
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
        qWarning() << WARNING_ZIPNOTSET;
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
        if (tools::isFolderItem(filename))
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
        qWarning() << WARNING_ZIPNOTSET;
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
    if (tools::isFolderItem(filename)) {
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
    if (!isModeReading()) {
        qWarning() << WARNING_WRONGMODE;
        return QByteArray();
    }

    return tools::extract_to_buffer(static_cast<mz_zip_archive *>(m_archive), index);
}

// This function is faster and consumes less resources than the previous one,
// but requires an additional delete operation to avoid memory leaks. ( delete _array.constData(); )
QByteArray QMicroz::extractDataRef(int index) const
{
    if (!isModeReading()) {
        qWarning() << WARNING_WRONGMODE;
        return QByteArray();
    }

    return tools::extract_to_buffer(static_cast<mz_zip_archive *>(m_archive),
                                    index, false);
}


/*** STATIC functions ***/
bool QMicroz::extract(const QString &zip_path)
{
    // extract to parent folder
    return extract(zip_path, QFileInfo(zip_path).absolutePath());
}

bool QMicroz::extract(const QString &zip_path, const QString &output_folder)
{
    QMicroz qmz(zip_path);
    if (!qmz)
        return false;

    qmz.setOutputFolder(output_folder);
    return qmz.extractAll();
}

/*** Compress ***/
bool QMicroz::compress(const QString &path)
{
    QFileInfo fi(path);

    if (!fi.isFile() && !fi.isDir()) {
        qWarning() << WARNING_WRONGPATH << path;
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
        qWarning() << WARNING_WRONGPATH << source_path;
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
        if (!fi.exists()) {
            qWarning() << "QMicroz: Skipped:" << path;
            continue;
        }

        worklist << path;

        if (fi.isDir())
            worklist << tools::folderContent(path);
    }

    if (worklist.isEmpty()) {
        qWarning() << WARNING_NOINPUTDATA;
        return false;
    }

    // create and open the output zip file
    mz_zip_archive *pZip = tools::za_new(zip_path, tools::ZaWriter);

    if (!pZip)
        return false;

    // process
    const bool res = tools::add_item_list(pZip, worklist, root);

    // success
    if (res)
        mz_zip_writer_finalize_archive(pZip);

    // cleanup
    tools::za_close(pZip);

    return res;
}

bool QMicroz::compress(const BufList &buf_data, const QString &zip_path)
{
    if (buf_data.isEmpty()) {
        qWarning() << WARNING_NOINPUTDATA;
        return false;
    }

    QMicroz qmz(zip_path);

    return qmz && qmz.addToZip(buf_data);
}

bool QMicroz::compress(const BufFile &buf_file, const QString &zip_path)
{
    if (!buf_file) {
        qWarning() << WARNING_NOINPUTDATA;
        return false;
    }

    QMicroz qmz(zip_path);

    return qmz && qmz.addToZip(buf_file);
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
