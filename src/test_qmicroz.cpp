/*
 * This file is part of QMicroz,
 * under the MIT License.
 * https://github.com/artemvlas/qmicroz
 *
 * Copyright (c) 2024 - present Artem Vlasenko
*/

#include <QtTest/QTest>

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
    void test_extract();
    void test_compress_file();
    void test_compress_folder();
    void test_data_integrity();

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
    QByteArray ba = "Random data to compress. 1234567890.";
    QString output_file = tmp_test_dir + "/test_compress_buf_file.zip";
    QMicroz::compress("compressed.txt", ba, output_file);

    QVERIFY(QMicroz::isZipFile(output_file));
}

void test_qmicroz::test_compress_buf_list()
{
    BufList buf_list = {
        { "empty_folder/", QByteArray() },
        { "file1.txt", "Random file data 1" },
        { "folder/file2.txt", "Random file data 2" },
        { "folder/file3.txt", "Random file data 3" },
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
    QVERIFY(qmz.isFolder(qmz.findIndex("empty_folder/")));
    QVERIFY(qmz.isFile(qmz.findIndex("file4.txt")));
    QVERIFY(qmz.isFile(qmz.findIndex("folder2/file5.txt")));
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

void test_qmicroz::test_data_integrity()
{
    QMicroz::extract(tmp_test_dir + "/test_compress_file.zip", tmp_test_dir + "/data_ckeck");

    QFile file(tmp_test_dir + "/data_ckeck/test_compress_file_(source).txt");

    QVERIFY(file.open(QFile::ReadOnly));
    const QByteArray readed = file.readAll();
    QVERIFY(readed == "Random data to file creating. 1234567890.");
}

QTEST_APPLESS_MAIN(test_qmicroz)

#include "test_qmicroz.moc"
