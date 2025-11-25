/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Copyright (c) 2024 - present Artem Vlasenko
*/

#include <QtTest/QTest>
#include <qtestcase.h>

#include "qmicroz.h"

class test_qmicroz : public QObject
{
    Q_OBJECT

public:
    test_qmicroz();
    ~test_qmicroz();

private slots:
    void test_compress_buf_file();
    void test_compress_buf_list();
    void test_extract_to_buf();
    void test_extract();
    void test_compress_file();
    void test_compress_folder();
    void test_compress_paths();
    void test_data_integrity();
    void test_addToZipPath();
    void test_addToZipPathEntryPath();
    void test_setZipWriting();
    void test_nestedFoldersCreation();

private:
    const QString tmp_test_dir = QDir::currentPath() + "/tmp_test_files";
};

test_qmicroz::test_qmicroz()
{
    QDir().mkdir(tmp_test_dir);
}

test_qmicroz::~test_qmicroz()
{
    QDir(tmp_test_dir).removeRecursively();
}

void test_qmicroz::test_compress_buf_file()
{
    // zip random data
    QByteArray ba = "Random data to compress. 1234567890.\nData. random, random 0987654321!\n";
    QString output_file = tmp_test_dir + "/test_compress_buf_file.zip";
    QDateTime dt = QDateTime::fromString("1999-06-21 11:23", "yyyy-MM-dd HH:mm");

    BufFile bufFile("compressed.txt", ba + ba);
    bufFile.modified = dt;

    QMicroz::compress(bufFile, output_file);

    QVERIFY(QMicroz::isZipFile(output_file));

    // open and test the created archive
    QMicroz qmz(output_file);
    QVERIFY(!qmz.contents().isEmpty());
    QVERIFY(qmz.isFile(0));
    QVERIFY(qmz.sizeCompressed(0) < qmz.sizeUncompressed(0));
    QCOMPARE(qmz.extractData(0), ba + ba);
    QCOMPARE(qmz.lastModified(0), dt);

    BufFile buf_folder("empty/");
    buf_folder.modified = dt;

    QString output_file2 = tmp_test_dir + "/test_compress_buf_folder.zip";
    QMicroz::compress(buf_folder, output_file2);

    QVERIFY(QMicroz::isZipFile(output_file2));
    QVERIFY(qmz.setZipFile(output_file2));
    QVERIFY(qmz.isFolder(0));
    QCOMPARE(qmz.lastModified(0), dt);
}

void test_qmicroz::test_compress_buf_list()
{
    BufList buf_list = {
        { "empty_folder/", QByteArray() },
        { "file1.txt", "Random file data 1" },
        { "folder/file2.txt", "Random file data 2" },
        { "folder/file3.txt", "Random file data 3" },
        { "folder/folder/file33.txt", "Random file data 33" },
        { "file4.txt", "Random file data 4" },
        { "folder2/file5.txt", "Random file data 5" },
        { "folder2/file6.txt", "Random file data 6" },
    };

    QString output_file = tmp_test_dir + "/test_compress_buf_list.zip";
    QMicroz::compress(buf_list, output_file);

    QVERIFY(QMicroz::isZipFile(output_file));

    QMicroz qmz(output_file);

    QVERIFY(qmz);
    QVERIFY(qmz.isFolder(0));
    QVERIFY(!qmz.isFile(0));
    QVERIFY(qmz.isFolder(qmz.findIndex("empty_folder/")));
    QVERIFY(qmz.isFile(qmz.findIndex("file4.txt")));
    QVERIFY(qmz.isFile(qmz.findIndex("folder2/file5.txt")));

    QString custom_output = tmp_test_dir + "/custom_folder/file111.txt";
    QVERIFY(qmz.extractFile("file1.txt", custom_output));
    QVERIFY(QFileInfo::exists(custom_output));
}

void test_qmicroz::test_extract_to_buf()
{
    QMicroz qmz(tmp_test_dir + "/test_compress_buf_list.zip", QMicroz::ModeRead);

    QVERIFY(qmz);

    BufList list = qmz.extractToBuf();
    QCOMPARE(list.size(), 7);
    QVERIFY(list.value("file4.txt") == "Random file data 4");
    QVERIFY(qmz.extractToBuf(0).name == "empty_folder/");

    BufFile buf_file = qmz.extractFileToBuf("file5.txt");
    QVERIFY(buf_file && buf_file.data == "Random file data 5");
}

void test_qmicroz::test_extract()
{
    QVERIFY(QMicroz::extract(tmp_test_dir + "/test_compress_buf_list.zip"));
    QVERIFY(QFileInfo::exists(tmp_test_dir + "/folder/file3.txt"));
    QVERIFY(QFileInfo::exists(tmp_test_dir + "/file4.txt"));
    QVERIFY(QFileInfo(tmp_test_dir + "/empty_folder").isDir());
    QVERIFY(QFileInfo(tmp_test_dir + "/folder").isDir());
}

void test_qmicroz::test_compress_file()
{
    QByteArray ba = "Random data to file creating. 1234567890.";
    QString input_file = tmp_test_dir + "/test_compress_file_(source).txt";
    QString output_file = tmp_test_dir + "/test_compress_file.zip";

    QFile file(input_file);
    file.open(QFile::WriteOnly) && file.write(ba);
    file.close();

    QMicroz::compress(input_file, output_file);

    QVERIFY(!QMicroz::isZipFile(input_file));
    QVERIFY(QMicroz::isZipFile(output_file));
}

void test_qmicroz::test_compress_folder()
{
    QString input_folder = tmp_test_dir + "/folder2";
    QString zip_path = input_folder + ".zip";

    QVERIFY(QMicroz::compress(input_folder));
    QVERIFY(QMicroz::isZipFile(zip_path));

    QMicroz qmz(zip_path);

    QVERIFY(qmz.isFolder(0));
    QVERIFY(qmz.isFile(1));
    QVERIFY(qmz.count() == 3);
    QVERIFY(qmz.findIndex("folder2/file6.txt") > 0);
    QVERIFY(qmz.findIndex("file5.txt") > 0);
    QVERIFY(qmz.findIndex("not_added_file.txt") == -1);
}

void test_qmicroz::test_compress_paths()
{
    QStringList paths;
    paths << (tmp_test_dir + "/folder");
    paths << (tmp_test_dir + "/folder2/file6.txt");
    paths << (tmp_test_dir + "/test_compress_file_(source).txt");
    paths << (tmp_test_dir + "/folder2/file5.txt");

    QString zip_path = tmp_test_dir + "/test_compress_paths.zip";
    QVERIFY(QMicroz::compress(paths, zip_path));

    QMicroz qmz(zip_path, QMicroz::ModeRead);
    QVERIFY(qmz.count() > 0);
    qDebug() << qmz.contents();
}

void test_qmicroz::test_data_integrity()
{
    QMicroz::extract(tmp_test_dir + "/test_compress_file.zip", tmp_test_dir + "/data_ckeck");

    QFile file(tmp_test_dir + "/data_ckeck/test_compress_file_(source).txt");

    QVERIFY(file.open(QFile::ReadOnly));
    const QByteArray readed = file.readAll();
    QVERIFY(readed == "Random data to file creating. 1234567890.");
}

void test_qmicroz::test_addToZipPath()
{
    QString zip_path = tmp_test_dir + "/test_test_addToZipPath.zip";
    QMicroz qmz(zip_path, QMicroz::ModeWrite);

    QVERIFY(qmz);
    //qmz.setVerbose(true);

    QVERIFY(qmz << (tmp_test_dir + "/empty_folder"));
    QVERIFY(!qmz.addToZip(tmp_test_dir + "/empty_folder"));
    QVERIFY(qmz.addToZip(tmp_test_dir + "/data_ckeck"));
    QVERIFY(!qmz.addToZip(tmp_test_dir + "/data_ckeck"));
    QVERIFY(qmz.addToZip(tmp_test_dir + "/folder2/file6.txt"));
    QVERIFY(!qmz.addToZip(tmp_test_dir + "/folder2/file6.txt"));
    QVERIFY(qmz << (tmp_test_dir + "/file4.txt"));
    QVERIFY(qmz << (tmp_test_dir + "/folder"));

    ZipContents content;
    content["empty_folder/"] = 0;
    content["data_ckeck/"] = 1;
    content["data_ckeck/test_compress_file_(source).txt"] = 2;
    content["file6.txt"] = 3;
    content["file4.txt"] = 4;
    content["folder/"] = 5;
    content["folder/file2.txt"] = 6;
    content["folder/file3.txt"] = 7;
    content["folder/folder/"] = 8;
    content["folder/folder/file33.txt"] = 9;

    QVERIFY(qmz.setZipFile(zip_path, QMicroz::ModeRead));
    QCOMPARE(qmz.contents(), content);
    QVERIFY(qmz.isFile(2));
    QVERIFY(qmz.isFolder(0));
    QCOMPARE(qmz.extractData(qmz.findIndex("file4.txt")), "Random file data 4");
}

void test_qmicroz::test_addToZipPathEntryPath()
{
    QMicroz qmz(tmp_test_dir + "/test_addToZipPathEntryPath.zip", QMicroz::ModeWrite);
    //qmz.setVerbose(true);

    QVERIFY(qmz);

    QVERIFY(qmz.addToZip(tmp_test_dir + "/empty_folder", "empty_folder"));
    QVERIFY(qmz.addToZip(tmp_test_dir + "/data_ckeck", "dataCkeck"));
    QVERIFY(qmz.addToZip(tmp_test_dir + "/folder2/file6.txt", "folder2/file6.txt"));
    QVERIFY(qmz.addToZip(tmp_test_dir + "/file4.txt", "file4.txt"));
    QVERIFY(qmz.addToZip(tmp_test_dir + "/folder/folder/file33.txt", "file55.txt"));
    QVERIFY(qmz.addToZip(tmp_test_dir + "/folder2/file6.txt", "fooFolder/file6.txt"));

    ZipContents content;
    content["empty_folder/"] = 0;
    content["dataCkeck/"] = 1;
    content["dataCkeck/test_compress_file_(source).txt"] = 2;
    content["folder2/file6.txt"] = 3;
    content["file4.txt"] = 4;
    content["file55.txt"] = 5;
    content["fooFolder/file6.txt"] = 6;

    qmz.closeArchive();

    QMicroz qmzRead(tmp_test_dir + "/test_addToZipPathEntryPath.zip");

    QCOMPARE(qmzRead.contents(), content);
    QVERIFY(qmzRead.isFile(2));
    QVERIFY(qmzRead.isFolder(0));
}

void test_qmicroz::test_setZipWriting()
{
    QMicroz qmz(tmp_test_dir + "/test_addToZipPathEntryPath.zip");
    QVERIFY(qmz && !qmz.isModeWriting());

    const QString file_path = tmp_test_dir + "/file4.txt";
    QVERIFY(QFileInfo::exists(file_path));
    QVERIFY(!QMicroz::isZipFile(file_path));

    QVERIFY(qmz.setZipFile(file_path, QMicroz::ModeWrite));

    qmz.addToZip(tmp_test_dir + "/file1.txt");
    qmz.closeArchive();

    QVERIFY(qmz.setZipFile(file_path) && qmz.isModeReading());
    QVERIFY(qmz.extractData(0) == "Random file data 1");

    QVERIFY(QMicroz::isZipFile(file_path));
    QVERIFY(qmz.setZipFile(file_path, QMicroz::ModeWrite));
    QVERIFY(qmz.addToZip(tmp_test_dir + "/file1.txt"));
    QVERIFY(qmz.addToZip(tmp_test_dir + "/file1.txt", "file2.txt"));
    QVERIFY(qmz.count() == 2);
    qmz.closeArchive();

    QVERIFY(qmz.setZipFile(file_path) && qmz.isModeReading());
    QVERIFY(qmz.extractData(0) == "Random file data 1");
    QVERIFY(qmz.extractData(0) == qmz.extractData(qmz.findIndex("file2.txt")));
}

void test_qmicroz::test_nestedFoldersCreation()
{
    QString zip_file = tmp_test_dir + "/test_nested_folder.zip";
    QMicroz qmz(zip_file, QMicroz::ModeWrite);

    // add to zip
    QVERIFY(qmz && qmz << BufFile("nested_folders_root/"));
    QVERIFY(qmz << BufFile("nested_folders_root/nested_folder_1/"));
    QVERIFY(qmz << BufFile("nested_folders_root/nested_folder_1/nested_folder2/"));

    // extract
    QVERIFY(qmz.setZipFile(zip_file, QMicroz::ModeRead));
    QVERIFY(qmz.count() == 3 && qmz.extractIndex(2));
    QVERIFY(qmz.extractIndex(1, tmp_test_dir + "/nested_folders_custom_root/nested_folder_1"));
    QVERIFY(qmz.extractIndex(1, tmp_test_dir + "/nested_folders_custom_root/nested_custom_folder/"));
    //QVERIFY(!qmz.extractIndex(5, tmp_test_dir + "/nested_folders_wrong_index"));

    QVERIFY(QFileInfo::exists(tmp_test_dir + "/nested_folders_root"));
    QVERIFY(QFileInfo::exists(tmp_test_dir + "/nested_folders_root/nested_folder_1/nested_folder2"));
    QVERIFY(QFileInfo::exists(tmp_test_dir + "/nested_folders_custom_root/nested_folder_1"));
    QVERIFY(QFileInfo::exists(tmp_test_dir + "/nested_folders_custom_root/nested_custom_folder/"));
    //QVERIFY(!QFileInfo::exists(tmp_test_dir + "/nested_folders_wrong_index"));
}

QTEST_APPLESS_MAIN(test_qmicroz)

#include "test_qmicroz.moc"
