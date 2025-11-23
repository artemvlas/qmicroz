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

// List of files { "entry name/path" : index } contained in the archive
using ZipContents = QMap<QString, int>;

class QMICROZ_EXPORT QMicroz
{
public:
    enum Mode { ModeAuto, ModeRead, ModeWrite };

    QMicroz();
    ~QMicroz();

    // To avoid ambiguity...
    explicit QMicroz(const char *zip_path, Mode mode = ModeAuto);

    // Sets <zip_path> and opens a new archive for Reading or Writing same as <setZipFile>
    explicit QMicroz(const QString &zip_path, Mode mode = ModeAuto);

    // existing zip archive buffered in memory
    explicit QMicroz(const QByteArray &buffered_zip);

    // Checks whether the archive is set
    explicit operator bool() const { return (bool)m_archive; }

    // The archive is set for Reading
    bool isModeReading() const;

    // ... for Writing
    bool isModeWriting() const;

    /* Sets and opens the zip for the current object.
     * ModeAuto
     * If <zip_path> is an existing zip archive, opens it for Reading.
     * If <zip_path> does not exist, opens for Writing.
     *
     * ModeRead
     * If <zip_path> is an existing zip archive, opens it for Reading.
     * Otherwise returns false.
     *
     * ModeWrite
     * Sets <zip_path> and opens the archive for Writing.
     * Regardless of the file existence.
     */
    bool setZipFile(const QString &zip_path, Mode mode = ModeAuto);

    // Sets a buffered in memory zip archive.
    bool setZipBuffer(const QByteArray &buffered_zip);

    // Path to the folder where to place the extracted files; empty --> parent dir
    void setOutputFolder(const QString &output_folder = QString());

    // Closes the currently opened zip and clears the member values
    void closeArchive();

    // Sets a more verbose output into the terminal (more text)
    void setVerbose(bool enable);

    /*** Info about the Archive ***/
    // returns the path to the current zip-file ("m_zip_path")
    const QString& zipFilePath() const;

    // The path to place the extracted files
    const QString& outputFolder() const;

    // Total uncompressed data size (space required for extraction)
    qint64 sizeUncompressed() const;


    /*** Zipped Items Info ***/
    // Returns a list of entries { "name/path" : index } contained in the archive
    const ZipContents& contents() const;

    // Returns the number of items in the archive
    int count() const;

    // Returns the index by the specified <file_name>, -1 if not found
    int findIndex(const QString &file_name) const;

    // Whether the specified index belongs to the folder
    bool isFolder(int index) const;

    // ... to the file
    bool isFile(int index) const;

    // Returns the name/path corresponding to the index
    QString name(int index) const;

    // Returns the compressed size of the file at the specified index
    qint64 sizeCompressed(int index) const;

    // ...the uncompressed size
    qint64 sizeUncompressed(int index) const;

    // Returns the file modification date stored in the archive
    QDateTime lastModified(int index) const;


    /*** Adding to the archive ***/
    // Adds a file or folder (including all contents) to the root of the archive
    bool addToZip(const QString &source_path);

    /* Adds a file or folder (including contents) to the <entry> name/path.
     * ("/home/folder/file.txt", "file.txt")        --> "file.txt"
     * ("/home/folder/file.txt", "folder/file.txt") --> "folder/file.txt"
     * ("/home/folder/file.txt", "newfile.txt")     --> "newfile.txt"
     * ("home/folder", "folder")                    --> "folder/", "folder/file.txt"
     * ("home/folder", "newfolder")                 --> "newfolder/", "newfolder/file.txt"
     */
    bool addToZip(const QString &source_path, const QString &entry_path);

    /* Adds a file based on <buf_file> data.
     * To add an empty folder entry, append '/' to the <buf_file.name>
     */
    bool addToZip(const BufFile &buf_file);

    // Adds files from the listed paths and data
    bool addToZip(const BufList &buf_data);


    /*** Extraction ***/
    // Extracts the entire contents of the archive into the output folder (the parent one if not set)
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
    // Extracts the zip into the parent folder
    static bool extract(const QString &zip_path);

    // ...to <output_folder>
    static bool extract(const QString &zip_path, const QString &output_folder);

    // Zips a file or folder <path>, places the output zip file to the parent folder
    static bool compress(const QString &path);

    // Zips a list of files and/or folders <paths>. Output zip path based on parent folder.
    static bool compress(const QStringList &paths);

    // Zips a file or folder <path>, output to <zip_path> file
    static bool compress(const QString &source_path, const QString &zip_path);

    // Zips a list of files and/or folders <paths>, output to <zip_path> file
    static bool compress(const QStringList &paths, const QString &zip_path);

    // Creates an archive with files from the listed paths and data
    static bool compress(const BufList &buf_data, const QString &zip_path);

    // Creates an archive containing a single file based on <buf_file> name and data
    static bool compress(const BufFile &buf_file, const QString &zip_path);

    /* Creates an archive <zip_path> containing a file (<file_name>, <file_data>)
     * <file_name> is the displayed file name inside the archive
     */
    static bool compress(const QString &file_name,
                         const QByteArray &file_data, const QString &zip_path);

    // Checks whether the <data> is an archive
    static bool isArchive(const QByteArray &data);

    // Checks the presence of a file, and whether it is an archive
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
    // Updates the list of current archive contents <m_zip_entries>
    const ZipContents& updateZipContents();

    // The void pointer is used to allow the miniz header not to be included
    void *m_archive = nullptr;

    // Whether to display more info into the terminal
    bool m_verbose = false;

    // Path to the current zip file
    QString m_zip_path;

    // Folder to place the extracted files
    QString m_output_folder;

    // Holds a list of current zip contents { "entry name/path" : index }
    ZipContents m_zip_entries;

    // Literal ".zip"
    static const QString s_zip_ext;

}; // class QMicroz

#endif // QMICROZ_H
