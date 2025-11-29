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
#include <QDirIterator>
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

    Mode zamode = ModeAuto; // 0

    switch (mode) {
    case ModeAuto:
        if (!QFileInfo::exists(zip_path))
            zamode = ModeWrite;
        else if (isZipFile(zip_path))
            zamode = ModeRead;
        break;
    case ModeRead:
        if (isZipFile(zip_path))
            zamode = ModeRead;
        break;
    case ModeWrite:
        zamode = ModeWrite;
        break;
    default:
        break;
    }

    if (zamode == 0) {
        qWarning() << WARNING_WRONGPATH << zip_path;
        return false;
    }

    // create and open a zip archive
    mz_zip_archive *pZip = new mz_zip_archive();
    const char* zapath = zip_path.toUtf8().constData();

    // Here the <zamode> can be either ModeRead or ModeWrite
    bool success = (zamode == ModeWrite) ? mz_zip_writer_init_file(pZip, zapath, 0)
                                         : mz_zip_reader_init_file(pZip, zapath, 0);

    if (!success) {
        qWarning() << "QMicroz: Failed to open zip file:" << zip_path;
        delete pZip;
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
        qWarning() << "QMicroz: No output folder.";

    return m_output_folder;
}

void QMicroz::closeArchive()
{
    if (!m_archive)
        return;

    mz_zip_archive *pZip = static_cast<mz_zip_archive *>(m_archive);

    if (isModeWriting())
        mz_zip_writer_finalize_archive(pZip);

    if (!mz_zip_end(pZip))
        qWarning() << "QMicroz: Failed to close archive.";

    delete pZip;
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
        if (!filename.isEmpty())
            m_zip_entries[filename] = it;
    }

    return m_zip_entries;
}

bool QMicroz::addEntry(const QString &entryName, std::function<bool()> addFunc)
{
    if (entryName.isEmpty())
        return false;

    QDebug deb = qDebug();

    if (m_verbose)
        deb << "Adding:" << entryName;

    if (m_zip_entries.contains(entryName)) {
        if (m_verbose)
            deb << "EXISTS";
        return false;
    }

    if (!addFunc()) {
        if (m_verbose)
            deb << "FAILED";
        return false;
    }

    if (m_verbose)
        deb << "OK";

    m_zip_entries[entryName] = m_zip_entries.size();
    return true;
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

    mz_zip_archive *pZip = static_cast<mz_zip_archive *>(m_archive);

    auto addFile = [pZip](const QString &source, const QString &entry) {
        std::function<bool()> func = [pZip, &source, &entry]() {
            return mz_zip_writer_add_file(pZip,                        // zip archive
                                          entry.toUtf8().constData(),  // entry name/path inside the zip
                                          source.toUtf8().constData(), // filesystem path
                                          NULL, 0,
                                          tools::compressLevel(QFileInfo(source).size()));
        };
        return func;
    };

    QFileInfo fi_source(source_path);

    if (fi_source.isFile()) {
        return addEntry(entry_path, addFile(source_path, entry_path));
    } else if (fi_source.isDir()) {
        // adding the folder entry itself
        bool added = addToZip(BufFile(tools::toFolderName(entry_path), fi_source.lastModified()));

        // adding folder contents
        QDir dir(source_path);

        QDirIterator it(source_path,
                        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::Readable,
                        QDirIterator::Subdirectories);

        while (it.hasNext()) {
            const QString fullPath = it.next();
            const QString relPath = tools::joinPath(entry_path, dir.relativeFilePath(fullPath));
            const QFileInfo &fi = it.fileInfo();

            if ((fi.isFile() && addEntry(relPath, addFile(fullPath, relPath)))
                || (fi.isDir() && addToZip(BufFile(tools::toFolderName(relPath), fi.lastModified()))))
            {
                added = true;
            }
        }

        return added;
    }

    return false;
}

bool QMicroz::addToZip(const BufFile &buf_file)
{
    if (!isModeWriting()) {
        qWarning() << WARNING_WRONGMODE;
        return false;
    }

    mz_zip_archive *pZip = static_cast<mz_zip_archive *>(m_archive);

    std::function<bool()> func = [pZip, &buf_file]() {
        const QByteArray &data = tools::isFolderName(buf_file.name) ? QByteArray() : buf_file.data;
        time_t modified = buf_file.modified.isValid() ? buf_file.modified.toSecsSinceEpoch() : 0;

        return mz_zip_writer_add_mem_ex_v2(pZip,
                                           buf_file.name.toUtf8().constData(),  // entry name/path
                                           data.constData(),                    // file data
                                           data.size(),                         // file size
                                           NULL, 0,
                                           tools::compressLevel(data.size()),
                                           0, 0,
                                           modified > 0 ? &modified : NULL,     // last modified, NULL to set current time
                                           NULL, 0, NULL, 0);
    }; // lambda

    return addEntry(buf_file.name, func);
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
    if (outputFolder().isEmpty())
        return false;

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

    auto createFolder = [](const QString &path) {
        if (QFileInfo::exists(path) || QDir().mkpath(path))
            return true;

        qWarning() << "QMicroz: Failed to create directory:" << path;
        return false;
    };

    if (tools::isFileName(filename)) {
        if (m_verbose)
            qDebug() << "Extracting:" << filename; // or "Extracting:" << (index + 1) << '/' << count() << filename;

        const QString parent_folder = QFileInfo(output_path).absolutePath();

        // create parent folder on disk if not any
        if (!createFolder(parent_folder))
            return false;

        // extracting...
        return tools::extract_to_file(static_cast<mz_zip_archive *>(m_archive), index, output_path);
    }

    // <filename> is a folder entry
    return createFolder(output_path);
}

bool QMicroz::extractFile(const QString &file_name)
{
    return extractIndex(findIndex(file_name));
}

bool QMicroz::extractFile(const QString &file_name, const QString &output_path)
{
    return extractIndex(findIndex(file_name), output_path);
}

bool QMicroz::extractFolder(int index)
{
    if (outputFolder().isEmpty())
        return false;

    return extractFolder(index, tools::joinPath(outputFolder(), name(index)));
}

bool QMicroz::extractFolder(int index, const QString &output_path)
{
    if (!isFolder(index))
        return false;

    bool extracted = false;
    QString folder_entry = name(index);
    ZipContents::const_iterator it = m_zip_entries.constBegin();

    for (; it != m_zip_entries.constEnd(); ++it) {
        if (it.key().startsWith(folder_entry)) {
            // e.g. "folder_entry/file" --> "file"
            QString relPath = it.key().mid(folder_entry.size());

            if (extractIndex(it.value(), tools::joinPath(output_path, relPath)))
                extracted = true;
            else
                qWarning() << "Failed to extract:" << it.key();
        }
    }

    return extracted;
}

BufList QMicroz::extractToBuf() const
{
    BufList res;

    if (!m_archive) {
        qWarning() << WARNING_ZIPNOTSET;
        return res;
    }

    // extracting...
    for (int it = 0; it < count(); ++it) {
        const QString filename = name(it);

        if (tools::isFileName(filename)) {
            if (m_verbose)
                qDebug() << "Extracting:" << (it + 1) << '/' << count() << filename;

            // extract file
            const QByteArray data = extractData(it);
            if (!data.isNull())
                res[filename] = data;
        }
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

    const QString filename = name(index);
    if (filename.isEmpty())
        return res;

    if (m_verbose)
        qDebug() << "Extracting:" << filename;

    res.name = filename;

    if (tools::isFileName(filename)) {
        // extract file
        const QByteArray ex_data = extractData(index);

        if (!ex_data.isNull()) {
            res.data = ex_data;
            res.modified = lastModified(index);

            if (m_verbose)
                qDebug() << "Unzipped:" << ex_data.size() << "bytes";
        }
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
