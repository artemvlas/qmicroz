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
#include <iostream>

const QString QMicroz::s_zip_ext = QStringLiteral(u".zip");

QMicroz::QMicroz() {}

QMicroz::QMicroz(const char* zipPath)
{
    setZipFile(QString(zipPath));
}

QMicroz::QMicroz(const QString &zipPath, Mode mode)
{
    setZipFile(zipPath, mode);
}

QMicroz::QMicroz(const QByteArray &bufferedZip)
{
    setZipBuffer(bufferedZip);
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

bool QMicroz::setZipFile(const QString &zipPath, Mode mode)
{
    // close the currently opened one if any
    closeArchive();

    Mode zamode = ModeAuto; // 0

    switch (mode) {
    case ModeAuto:
        if (!QFileInfo::exists(zipPath))
            zamode = ModeWrite;
        else if (isZipFile(zipPath))
            zamode = ModeRead;
        break;
    case ModeRead:
        if (isZipFile(zipPath))
            zamode = ModeRead;
        break;
    case ModeWrite:
        zamode = ModeWrite;
        break;
    default:
        break;
    }

    if (zamode == 0) {
        qWarning() << WARNING_WRONGPATH << zipPath;
        return false;
    }

    // create and open a zip archive
    mz_zip_archive *pZip = new mz_zip_archive();
    const char* zapath = zipPath.toUtf8().constData();

    // Here the <zamode> can be either ModeRead or ModeWrite
    bool success = (zamode == ModeWrite) ? mz_zip_writer_init_file(pZip, zapath, 0)
                                         : mz_zip_reader_init_file(pZip, zapath, 0);

    if (!success) {
        qWarning() << "QMicroz: Failed to open zip file:" << zipPath;
        delete pZip;
        return false;
    }

    m_archive = pZip;
    m_zip_path = zipPath;

    if (isModeReading()) {
        updateZipContents();
        setOutputFolder(); // zip file's parent folder
    }

    return true;
}

bool QMicroz::setZipBuffer(const QByteArray &bufferedZip)
{
    if (!isArchive(bufferedZip)) {
        qWarning() << "QMicroz: The byte array is not zipped";
        return false;
    }

    // open zip archive
    mz_zip_archive *pZip = new mz_zip_archive();

    if (mz_zip_reader_init_mem(pZip, bufferedZip.constData(), bufferedZip.size(), 0)) {
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

void QMicroz::setOutputFolder(const QString &outputFolder)
{
    if (outputFolder.isEmpty() && !m_zip_path.isEmpty()) {
        // set zip file's parent folder
        m_output_folder = QFileInfo(m_zip_path).absolutePath();
        return;
    }

    m_output_folder = outputFolder;
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

    if (m_verbose)
        std::cout << "Adding: " << entryName.toStdString();

    if (m_zip_entries.contains(entryName)) {
        if (m_verbose)
            std::cout << " " << "EXISTS" << std::endl;
        return false;
    }

    if (!addFunc()) {
        if (m_verbose)
            std::cout << " " << "FAILED" << std::endl;
        return false;
    }

    if (m_verbose)
        std::cout << " " << "OK" << std::endl;

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

int QMicroz::findIndex(const QString &fileName) const
{
    // full path matching
    if (m_zip_entries.contains(fileName))
        return m_zip_entries.value(fileName);

    // deep search, matching only the name, e.g. "file.txt" for "folder/file.txt"
    if (!fileName.contains(tools::s_sep)) {
        ZipContents::const_iterator it;

        for (it = m_zip_entries.constBegin(); it != m_zip_entries.constEnd(); ++it) {
            if (tools::isFileName(it.key())
                && fileName == QFileInfo(it.key()).fileName())
            {
                return it.value();
            }
        }
    }

    qDebug() << "QMicroz: Index not found:" << fileName;
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

bool QMicroz::addToZip(const QString &sourcePath)
{
    if (!QFileInfo::exists(sourcePath))
        return false;

    return addToZip(sourcePath, QFileInfo(sourcePath).fileName());
}

bool QMicroz::addToZip(const QString &sourcePath, const QString &entryName)
{
    if (!m_archive || entryName.isEmpty() || !QFileInfo::exists(sourcePath)) {
        return false;
    }

    if (!isModeWriting()) {
        qWarning() << WARNING_WRONGMODE;
        return false;
    }

    /* <source> is a path to the file on the file system.
     * <entry> its name or path inside the archive.
     */
    auto addFile = [this](const QString &source, const QString &entry) {
        mz_zip_archive *pZip = static_cast<mz_zip_archive *>(this->m_archive);

        std::function<bool()> func = [pZip, &source, &entry]() {
            return mz_zip_writer_add_file(pZip,                        // zip archive
                                          entry.toUtf8().constData(),  // entry name/path inside the zip
                                          source.toUtf8().constData(), // filesystem path
                                          NULL, 0,
                                          tools::compressLevel(QFileInfo(source).size()));
        };

        return this->addEntry(entry, func);
    }; // lambda addFile -> bool

    /* <entry> is a name or path of the folder inside the archive.
     * <modified> its last modified date; invalid QDateTime() to set current.
     */
    auto addFolder = [this](const QString &entry, const QDateTime &modified) {
        BufFile bf(tools::toFolderName(entry), modified);
        return this->addToZip(bf);
    }; // lambda addFolder -> bool

    QFileInfo fi_source(sourcePath);

    if (fi_source.isFile()) {
        return addFile(sourcePath, entryName);
    } else if (fi_source.isDir()) {
        // adding the folder entry itself
        bool added = addFolder(entryName, fi_source.lastModified());

        // adding folder contents
        QDir dir(sourcePath);

        QDirIterator it(sourcePath,
                        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden | QDir::Readable,
                        QDirIterator::Subdirectories);

        while (it.hasNext()) {
            const QString fullPath = it.next();
            const QString relPath = tools::joinPath(entryName, dir.relativeFilePath(fullPath));
            const QFileInfo &fi = it.fileInfo();

            if ((fi.isFile() && addFile(fullPath, relPath))
                || (fi.isDir() && addFolder(relPath, fi.lastModified())))
            {
                added = true;
            }
        }

        return added;
    }

    return false;
}

bool QMicroz::addToZip(const BufFile &bufFile)
{
    if (!isModeWriting()) {
        qWarning() << WARNING_WRONGMODE;
        return false;
    }

    mz_zip_archive *pZip = static_cast<mz_zip_archive *>(m_archive);

    std::function<bool()> func = [pZip, &bufFile]() {
        const QByteArray &data = tools::isFolderName(bufFile.name) ? QByteArray() : bufFile.data;
        time_t modified = bufFile.modified.isValid() ? bufFile.modified.toSecsSinceEpoch() : 0;

        return mz_zip_writer_add_mem_ex_v2(pZip,
                                           bufFile.name.toUtf8().constData(),   // entry name/path
                                           data.constData(),                    // file data
                                           data.size(),                         // file size
                                           NULL, 0,
                                           tools::compressLevel(data.size()),
                                           0, 0,
                                           modified > 0 ? &modified : NULL,     // last modified, NULL to set current time
                                           NULL, 0, NULL, 0);
    }; // lambda

    return addEntry(bufFile.name, func);
}

bool QMicroz::addToZip(const BufList &bufList)
{
    BufList::const_iterator it;
    for (it = bufList.constBegin(); it != bufList.constEnd(); ++it) {
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

    for (int i = 0; i < count(); ++i) {
        if (!extractIndex(i))
            return false;
    }

    return true;
}

bool QMicroz::extractIndex(int index)
{
    if (outputFolder().isEmpty())
        return false;

    return extractIndex(index, tools::joinPath(outputFolder(), name(index)));
}

bool QMicroz::extractIndex(int index, const QString &outputPath)
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
            std::cout << "Extracting: " << filename.toStdString(); // or "Extracting:" << (index + 1) << '/' << count() << filename;

        const QString parent_folder = QFileInfo(outputPath).absolutePath();

        // create parent folder on disk if not any
        if (!createFolder(parent_folder))
            return false;

        // extracting...
        mz_zip_archive *pZip = static_cast<mz_zip_archive *>(m_archive);
        bool res = mz_zip_reader_extract_to_file(pZip, index, outputPath.toUtf8().constData(), 0);

        if (m_verbose)
            std::cout << " " << (res ? "OK" : "FAILED") << std::endl;

        if (!res)
            qWarning() << "QMicroz: Failed to extract file:" << index << filename;

        return res;
    }

    // <filename> is a folder entry
    return createFolder(outputPath);
}

bool QMicroz::extractFile(const QString &fileName)
{
    return extractIndex(findIndex(fileName));
}

bool QMicroz::extractFile(const QString &fileName, const QString &outputPath)
{
    return extractIndex(findIndex(fileName), outputPath);
}

bool QMicroz::extractFolder(int index)
{
    if (outputFolder().isEmpty())
        return false;

    return extractFolder(index, tools::joinPath(outputFolder(), name(index)));
}

bool QMicroz::extractFolder(int index, const QString &outputPath)
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

            if (extractIndex(it.value(), tools::joinPath(outputPath, relPath)))
                extracted = true;
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
            // extract file
            const QByteArray data = extractData(it);
            if (!data.isNull())
                res[filename] = data;
        }
    }

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

    res.name = filename;
    res.modified = lastModified(index);

    if (tools::isFileName(filename)) {
        // extract file data
        res.data = extractData(index);
    }

    return res;
}

BufFile QMicroz::extractFileToBuf(const QString &fileName) const
{
    return extractToBuf(findIndex(fileName));
}

QByteArray QMicroz::extractData(int index) const
{
    // Pointer to data
    QByteArray extrRef = extractDataRef(index);

    if (extrRef.isNull())
        return QByteArray();

    // Copy data
    QByteArray extrCopy(extrRef.constData(), extrRef.size());

    // Clear extracted from the heap
    delete extrRef.constData();

    return extrCopy;
}

QByteArray QMicroz::extractDataRef(int index) const
{
    if (!isModeReading()) {
        qWarning() << WARNING_WRONGMODE;
        return QByteArray();
    }

    if (m_verbose)
        std::cout << "Extracting: " << name(index).toStdString();

    // extracting...
    mz_zip_archive *pZip = static_cast<mz_zip_archive *>(m_archive);

    size_t data_size = 0;
    char *ch_data = (char*)mz_zip_reader_extract_to_heap(pZip, index, &data_size, 0);

    // Pointer to the data in the QByteArray.
    // The Data should be deleted on the caller side: delete ba.constData();
    QByteArray extracted = ch_data ? QByteArray::fromRawData(ch_data, data_size) : QByteArray();

    if (m_verbose)
        std::cout << " " << (ch_data ? "OK" : "FAILED") << std::endl;

    return extracted;
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

bool QMicroz::compress(const BufList &buf_list, const QString &zip_path)
{
    if (buf_list.isEmpty()) {
        qWarning() << WARNING_NOINPUTDATA;
        return false;
    }

    QMicroz qmz(zip_path, ModeWrite);

    return qmz && qmz.addToZip(buf_list);
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
