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

QMicroz::QMicroz(const QString &zip_path, Mode mode)
{
    setZipFile(zip_path, mode);
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

bool QMicroz::setZipFile(const QString &zip_path, Mode mode)
{
    // close the currently opened one if any
    closeArchive();

    mz_zip_archive *pZip = nullptr;

    switch (mode) {
    case ModeAuto:
        if (!QFileInfo::exists(zip_path))
            pZip = tools::za_new(zip_path, tools::ZaWriter);
        else if (isZipFile(zip_path))
            pZip = tools::za_new(zip_path, tools::ZaReader);
        break;
    case ModeRead:
        if (isZipFile(zip_path))
            pZip = tools::za_new(zip_path, tools::ZaReader);
        break;
    case ModeWrite:
        pZip = tools::za_new(zip_path, tools::ZaWriter);
        break;
    default:
        break;
    }

    if (!pZip) {
        qWarning() << WARNING_WRONGPATH << zip_path;
        return false;
    }

    m_archive = pZip;
    m_zip_path = zip_path;

    if (isModeReading()) {
        updateZipContents();
        setOutputFolder(); // zip file's parent folder
    }

    return true;
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
    m_zip_entries.clear();
    m_zip_path.clear();
    m_output_folder.clear();
}

const ZipContents& QMicroz::updateZipContents()
{
    m_zip_entries.clear();

    if (!m_archive)
        return m_zip_entries;

    // iterating...
    for (int it = 0; it < count(); ++it) {
        const QString filename = name(it);
        if (filename.isEmpty())
            break;

        m_zip_entries[filename] = it;
    }

    return m_zip_entries;
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
    return m_zip_entries;
}

int QMicroz::count() const
{
    if (!m_archive)
        return 0;

    return mz_zip_reader_get_num_files(static_cast<mz_zip_archive *>(m_archive));
}

int QMicroz::findIndex(const QString &file_name) const
{
    // full path matching
    if (m_zip_entries.contains(file_name))
        return m_zip_entries.value(file_name);

    ZipContents::const_iterator it;

    // deep search, matching only the name
    if (!file_name.contains(tools::s_sep)) {
        for (it = m_zip_entries.constBegin(); it != m_zip_entries.constEnd(); ++it) {
            if (tools::isFileName(it.key())
                && file_name == QFileInfo(it.key()).fileName())
            {
                return it.value();
            }
        }
    }

    qDebug() << "QMicroz: Index not found:" << file_name;
    return -1;
}

bool QMicroz::isFolder(int index) const
{
    return tools::isFolderName(name(index));
}

bool QMicroz::isFile(int index) const
{
    return tools::isFileName(name(index));
}

QString QMicroz::name(int index) const
{
    return tools::za_file_stat(m_archive, index).m_filename;
}

qint64 QMicroz::sizeCompressed(int index) const
{
    if (!m_archive)
        return 0;

    return tools::za_file_stat(m_archive, index).m_comp_size;
}

qint64 QMicroz::sizeUncompressed(int index) const
{
    if (!m_archive)
        return 0;

    return tools::za_file_stat(m_archive, index).m_uncomp_size;
}

QDateTime QMicroz::lastModified(int index) const
{
    if (!m_archive)
        return QDateTime();

    const qint64 sec = tools::za_file_stat(m_archive, index).m_time;

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
    if (!m_archive || entry_path.isEmpty() || !QFileInfo::exists(source_path)) {
        return false;
    }

    if (!isModeWriting()) {
        qWarning() << WARNING_WRONGMODE;
        return false;
    }

    QFileInfo fi_source(source_path);
    mz_zip_archive *pZip = static_cast<mz_zip_archive *>(m_archive);

    if (fi_source.isFile()) {
        if (m_zip_entries.contains(entry_path))
            return false;

        bool res = tools::add_item_file(pZip, source_path, entry_path);
        if (res)
            m_zip_entries[entry_path] = m_zip_entries.size();

        return res;
    } else if (fi_source.isDir()) {
        QMap<QString, QString> folderContent;
        folderContent.insert(source_path, QString());
        folderContent.insert(tools::folderContentRel(source_path));

        bool added = false;

        // parsing a list of paths
        QMap<QString, QString>::const_iterator it;
        for (it = folderContent.constBegin(); it != folderContent.constEnd(); ++it) {
            QFileInfo fi(it.key());
            const QString relPath = tools::joinPath(entry_path, it.value());

            // TODO: checking existing entries <m_zip_entries.contains> needs to be automated.
            // existence check
            if (fi.isFile() && m_zip_entries.contains(relPath)
                || fi.isDir() && m_zip_entries.contains(tools::toFolderName(relPath)))
            {
                if (m_verbose)
                    qDebug() << "Exists:" << relPath;
                continue;
            }

            if (m_verbose)
                qDebug() << "Adding:" << relPath;

            // adding item
            if (fi.isFile() && tools::add_item_file(pZip, it.key(), relPath))
                m_zip_entries[relPath] = m_zip_entries.size();
            else if (fi.isDir() && tools::add_item_folder(pZip, relPath))
                m_zip_entries[tools::toFolderName(relPath)] = m_zip_entries.size();
            else // adding failed
                return false;

            added = true;
        }

        return added;
    }

    return false;
}

bool QMicroz::addToZip(const BufFile &buf_file)
{
    if (m_zip_entries.contains(buf_file.name))
        return false;

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
        m_zip_entries[buf_file.name] = m_zip_entries.size();

    return res;
}

bool QMicroz::addToZip(const BufList &buf_data)
{
    BufList::const_iterator it;
    for (it = buf_data.constBegin(); it != buf_data.constEnd(); ++it) {
        if (addToZip(BufFile(it.key(), it.value())))
            m_zip_entries[it.key()] = m_zip_entries.size();
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

bool QMicroz::extractIndex(int index)
{
    if (outputFolder().isEmpty()) {
        qWarning() << "QMicroz: No output folder.";
        return false;
    }

    return extractIndex(index, tools::joinPath(outputFolder(), name(index)));
}

bool QMicroz::extractIndex(int index, const QString &output_path)
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

    // the name is also a path inside the archive
    const QString filename = name(index);
    if (filename.isEmpty())
        return false;

    if (tools::isFileName(filename)) {
        if (m_verbose)
            qDebug() << "Extracting:" << filename; // or "Extracting:" << (index + 1) << '/' << count() << filename;

        const QString parent_folder = QFileInfo(output_path).absolutePath();

        // create parent folder on disk if not any
        if (!tools::createFolder(parent_folder))
            return false;

        // extracting...
        return tools::extract_to_file(static_cast<mz_zip_archive *>(m_archive), index, output_path);
    }

    // <filename> is a folder entry
    return tools::createFolder(output_path);
}

bool QMicroz::extractFile(const QString &file_name)
{
    return extractIndex(findIndex(file_name));
}

bool QMicroz::extractFile(const QString &file_name, const QString &output_path)
{
    return extractIndex(findIndex(file_name), output_path);
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
        if (tools::isFolderName(filename))
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
    if (tools::isFolderName(filename)) {
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
    QMicroz qmz(zip_path, ModeRead);
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
    if (paths.isEmpty()) {
        qWarning() << WARNING_NOINPUTDATA;
        return false;
    }

    /* At the moment we consider the parent folder of the first item in the list as the relative Root.
     * Archive entries are created relative to this root.
     * Paths from the list that do not have a common root are placed in the root of the archive.
     *
     * TODO: a smarter function for parsing paths and building internal entries based on multiple roots is needed.
     */
    const QString root = QFileInfo(paths.first()).absolutePath();
    QDir dir(root);

    QMicroz qmz(zip_path, ModeWrite);
    if (!qmz)
        return false;

    // process
    for (const QString &path : paths) {
        QString relPath = path.startsWith(root) ? dir.relativeFilePath(path)
                                                : QFileInfo(path).fileName();

        if (!qmz.addToZip(path, relPath))
            qWarning() << "QMicroz: Unable to add:" << path;
    }

    return qmz.count() > 0;
}

bool QMicroz::compress(const BufList &buf_data, const QString &zip_path)
{
    if (buf_data.isEmpty()) {
        qWarning() << WARNING_NOINPUTDATA;
        return false;
    }

    QMicroz qmz(zip_path, ModeWrite);

    return qmz && qmz.addToZip(buf_data);
}

bool QMicroz::compress(const BufFile &buf_file, const QString &zip_path)
{
    if (!buf_file) {
        qWarning() << WARNING_NOINPUTDATA;
        return false;
    }

    QMicroz qmz(zip_path, ModeWrite);

    return qmz && qmz.addToZip(buf_file);
}

bool QMicroz::compress(const QString &file_name,
                       const QByteArray &file_data,
                       const QString &zip_path)
{
    BufFile buf_file(file_name, file_data);

    return compress(buf_file, zip_path);
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
