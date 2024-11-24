### QMicroz is a simple Zip/Unzip solution for Qt projects.
This is a lightweight C++/Qt wrapper around the [miniz](https://github.com/richgel999/miniz) zip library (based on zlib).
The code can easily be compiled (or included) and used as a shared (or static) library, which will provide ready-made functions for quick and convenient work with zip archives. There is no need to add additional dependencies like zlib and deal with low-level APIs.

### Key features:
* Create and extract zip files.
* Working with files, folders and their lists.
* Create and extract zip archives from buffered data (QByteArrays).
* UnZipping files into the RAM buffer. UnZipiing buffered zip to disk.

### How to use:
Basic functions are available statically.
For example:

// extracting the zip archive into the parent folder
#### QMicroz::extract("zip_file_path");

// archiving a folder with custom path to zip
#### QMicroz::compress_folder("source_path", "zip_file_path");

// archiving a list of files and/or folders
#### QMicroz::compress_({"file_path", "folder_path", "file2_path"});

A complete list of functions is available in the [qmicroz.h](src/qmicroz.h) file.

### How to link a compiled library to an existing project:
Trivial example. Windows system, you have ready-made files: _libqmicroz.dll, libqmicroz.dll.a, qmicroz.h, qmicroz_global.h_\
Just create a folder in the root of your Qt project (for example **'lib'**) and copy these files into it.

And add to CMakeLists.txt:
```
include_directories("lib")
link_directories("lib")

target_link_libraries(${PROJECT_NAME} PRIVATE qmicroz)
```
And then in your *.cpp file:
```
#include "qmicroz.h"
```
