# QMicroz
### _A simple Zip/Unzip solution for Qt projects._
This is a lightweight (still powerful) C++/Qt wrapper around the [miniz](https://github.com/richgel999/miniz) zip library.\
The code can easily be used as a standalone library (shared or static), which will provide ready-made functions for quick and convenient work with zip archives.
There is no need to add additional dependencies and deal with low-level APIs.

_For basic and most common tasks, there are high-level static functions that allow you to get by with a single code line._
_For more flexible creation and extraction of archives, there are Qt-style OOP functions._

### The project purpose.
Creation of a minimalist, publicly accessible and most easy to use tool that does not require a long documentation study. The priority is the possibility of embedding into Qt projects with ease, with a reduced number of code lines.

The library is well suitable, for example, for working with the contents of the common document types (*.epub, *.docx, *.odt ...), which are essentially packaged archives.

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

#### Unzip to parent folder
    QMicroz::extract("zip_file_path");

#### Archive a file or folder with a custom output path
    QMicroz::compress("source_path", "zip_file_path");

#### Zip a list of files and/or folders
    QMicroz::compress({"file_path", "folder_path", "file2_path"});

#### Extract a file to the memory buffer (non-static)
```
BufList buffer;
QMicroz qmz("path_to_zip");
if (qmz) // whether successfully opened
    buffer = qmz.extractToBuf();
```

More examples on [Wiki](https://github.com/artemvlas/qmicroz/wiki/Usage-examples).\
A complete list of functions is available in the [qmicroz.h](src/qmicroz.h) file.
