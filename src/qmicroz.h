/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Copyright (c) 2024 Artem Vlasenko
*/

#ifndef QMICROZ_H
#define QMICROZ_H

#include <QtCore/qglobal.h>

#if defined(MAKE_SHARED)
#define QMICROZ_EXPORT Q_DECL_EXPORT
#pragma message("Symbols Export is Enabled")
#else
#define QMICROZ_EXPORT
#endif

#include <QStringList>
#include <QMap>
#include <QDateTime>

// Used to store a file data in the memory
struct QMICROZ_EXPORT BufFile {
    BufFile() {}
    BufFile(const QString &fileName, const QByteArray &fileData = QByteArray())
        : name(fileName), data(fileData) {}

    explicit operator bool() const { return !name.isEmpty(); }
    qint64 size() const { return data.size(); }

    QString name;       // file name (path inside the archive)
    QByteArray data;    // file data (uncompressed)
    QDateTime modified; // last modified date and time
}; // struct BufFile

// { "path inside zip" : data }
using BufList = QMap<QString, QByteArray>;

// list of files {index : path} contained in the archive
using ZipContents = QMap<int, QString>;

class QMICROZ_EXPORT QMicroz
{
public:
    QMicroz();
    ~QMicroz();

    // to avoid ambiguity
    explicit QMicroz(const char *zip_path);

    // path to existing zip file
    explicit QMicroz(const QString &zip_path);

    // existing zip archive buffered in memory
    explicit QMicroz(const QByteArray &buffered_zip);

    // checks whether the archive is set
    explicit operator bool() const { return (bool)m_archive; }

    // sets and opens the zip for the current object
    bool setZipFile(const QString &zip_path);

    // sets a buffered in memory zip archive
    bool setZipBuffer(const QByteArray &buffered_zip);

    // path to the folder where to place the extracted files; empty --> parent dir
    void setOutputFolder(const QString &output_folder = QString());

    // closes the currently setted zip and clears the member values
    void closeArchive();

    // sets a more verbose output into the terminal (more text)
    void setVerbose(bool enable);

    /*** Info about the Archive ***/
    // returns the path to the current zip-file ("m_zip_path")
    const QString& zipFilePath() const;

    // the path to place the extracted files
    const QString& outputFolder() const;

    // total uncompressed data size (space required for extraction)
    qint64 sizeUncompressed() const;

    /*** Zipped Items Info ***/
    // returns a list of files {index : path} contained in the archive
    const ZipContents& contents() const;

    // returns the number of items in the archive
    int count() const;

    // returns the index by the specified <file_name>, -1 if not found
    int findIndex(const QString &file_name) const;

    // whether the specified index belongs to the folder
    bool isFolder(int index) const;

    // ... to the file
    bool isFile(int index) const;

    // returns the name/path corresponding to the index
    QString name(int index) const;

    // returns the compressed size of the file at the specified index
    qint64 sizeCompressed(int index) const;

    // the uncompressed size
    qint64 sizeUncompressed(int index) const;

    // returns the file modification date stored in the archive
    QDateTime lastModified(int index) const;

    /*** Extraction ***/
    // extracts the archive into the output folder (or the parent one)
    bool extractAll();

    // extracts the file with index to disk
    bool extractIndex(int index, bool recreate_path = true);

    // finds the <file_name> and extracts if any; slower than 'extractIndex'
    bool extractFile(const QString &file_name, bool recreate_path = true);

    // unzips all files into the RAM buffer { path : data }
    BufList extractToBuf() const;

    // extracts the selected index only
    BufFile extractToBuf(int index) const;

    // finds the <file_name> and extracts to Buf; slower than (index)
    BufFile extractFileToBuf(const QString &file_name) const;

    // returns the extracted file data; QByteArray owns the copied data
    QByteArray extractData(int index) const;

    // QByteArray does NOT own the data! To free memory: delete _array.constData();
    QByteArray extractDataRef(int index) const;


    /*** STATIC functions ***/
    // extracting the zip into the parent folder
    static bool extract(const QString &zip_path);

    // to 'output_folder'
    static bool extract(const QString &zip_path, const QString &output_folder);

    // zip a file or folder <path>, place output zip file to the parent folder
    static bool compress(const QString &path);

    // zip a list of files and/or folders <paths>, output zip name and location based on parent folder
    static bool compress(const QStringList &paths);

    // zip a file or folder <path>, output to <zip_path> file
    static bool compress(const QString &source_path, const QString &zip_path);

    // zip a list of files and/or folders <paths>, output to <zip_path> file
    static bool compress(const QStringList &paths, const QString &zip_path);

    // creates an archive with files from the listed paths and data
    static bool compress(const BufList &buf_data, const QString &zip_path);

    // creates an archive containing single file based on <buf_file> name and data
    static bool compress(const BufFile &buf_file, const QString &zip_path);

    // creates an archive <zip_path> containing a file (<file_name>, <file_data>)
    // <file_name> is the displayed file name inside the archive
    static bool compress(const QString &file_name,
                         const QByteArray &file_data, const QString &zip_path);

    // checks whether this <data> is an archive
    static bool isArchive(const QByteArray &data);

    // checks the presence of a file, and whether it is an archive
    static bool isZipFile(const QString &filePath);


    /*** OBSOLETE ***/
    [[deprecated("Use QMicroz::compress(...) instead.")]]
    static bool compress_here(const QString &path);
    [[deprecated("Use QMicroz::compress(...) instead.")]]
    static bool compress_here(const QStringList &paths);
    [[deprecated("Use QMicroz::compress(...) instead.")]]
    static bool compress_file(const QString &source_path);
    [[deprecated("Use QMicroz::compress(...) instead.")]]
    static bool compress_file(const QString &source_path, const QString &zip_path);
    [[deprecated("Use QMicroz::compress(...) instead.")]]
    static bool compress_folder(const QString &source_path);
    [[deprecated("Use QMicroz::compress(...) instead.")]]
    static bool compress_folder(const QString &source_path, const QString &zip_path);
    [[deprecated("Use QMicroz::compress(...) instead.")]]
    static bool compress_list(const QStringList &paths, const QString &zip_path);
    [[deprecated("Use QMicroz::compress(...) instead.")]]
    static bool compress_buf(const BufList &buf_data, const QString &zip_path);
    [[deprecated("Use QMicroz::compress(...) instead.")]]
    static bool compress_buf(const QByteArray &data,
                             const QString &file_name, const QString &zip_path);
    /*** OBSOLETE ***/

private:
    // updates the list of current archive contents
    const ZipContents& updateZipContents();

    // Default hint in case the 'm_archive' is not set
    void warningZipNotSet() const;

    // the void pointer is used to allow the miniz header not to be included
    void *m_archive = nullptr;

    // whether to display more info into the terminal
    bool m_verbose = false;

    // path to the current zip file
    QString m_zip_path;

    // folder to place the extracted files
    QString m_output_folder;

    // holds the list of current contents { index : filename (or path) }
    ZipContents m_zip_contents;

    // ".zip"
    static const QString s_zip_ext;

}; // class QMicroz

#endif // QMICROZ_H
