# QMicroz
### _A simple Zip/Unzip solution for Qt projects._
This is a lightweight C++/Qt wrapper around the [miniz](https://github.com/richgel999/miniz) zip library (based on _zlib_).\
The code can easily be used as a standalone library (shared or static), which will provide ready-made functions for quick and convenient work with zip archives.
There is no need to add additional dependencies and deal with low-level APIs.

### The project purpose.
Creation of a minimalist, publicly accessible and most easy to use tool that does not require a long documentation study. The priority is the possibility of embedding into Qt projects with ease, with a reduced number of code lines.

The library is well suitable, for example, for working with the contents of files of common types of documents (*.epub, *.docx, *.odt, *.ver ...), which are essentially packaged archives.

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
BufList buffer;
QMicroz qmz("path_to_zip");
if (qmz) // whether successfully opened
    buffer = qmz.extractToBuf();
```

More examples on [Wiki](https://github.com/artemvlas/qmicroz/wiki/Usage-examples).\
A complete list of functions is available in the [qmicroz.h](src/qmicroz.h) file.
