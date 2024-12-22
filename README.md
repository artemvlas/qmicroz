# QMicroz
### _A simple Zip/Unzip solution for Qt projects._
This is a lightweight C++/Qt wrapper around the [miniz](https://github.com/richgel999/miniz) zip library (based on zlib).\
The code can easily be used as a library (shared or static), which will provide ready-made functions for quick and convenient work with zip archives.
There is no need to add additional dependencies like zlib and deal with low-level APIs.

### Key features:
* Create and extract zip files.\
    _Working with files, folders and their lists._
* Create/extract archives from/to **buffered data.**\
    _UnZip files into the RAM buffer.\
     Make a zip file from QByteArray (or a list of them).\
     Extract the buffered zip to disk..._

---
### How to use:
Basic functions are available statically.
For example:

#### Extracting the archive to the parent folder
    QMicroz::extract("zip_file_path");

#### Archiving a folder with custom path to zip
    QMicroz::compress_folder("source_path", "zip_file_path");

#### Zipping a list of files and/or folders
    QMicroz::compress_here({"file_path", "folder_path", "file2_path"});

#### UnZipping a file into a memory buffer (non-static)
```
BufList _buffer;
QMicroz _qmz("path_to_zip");
if (_qmz)
    _buffer = _qmz.extractToBuf();
```

More examples on [Wiki](https://github.com/artemvlas/qmicroz/wiki/Usage-examples).\
A complete list of functions is available in the [qmicroz.h](src/qmicroz.h) file.
