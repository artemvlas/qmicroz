cmake_minimum_required(VERSION 3.16)

project(qmicroz VERSION 0.3)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# find packages
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# include directories
include_directories("src" "miniz")

add_library(qmicroz SHARED
  src/qmicroz.h
  src/qmicroz.cpp
  src/tools.h
  src/tools.cpp
  miniz/miniz.h
  miniz/miniz.c
)

target_link_libraries(qmicroz PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_compile_definitions(qmicroz PRIVATE QMICROZ_LIBRARY)

# install [/usr/lib/libqmicroz.so]
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_INSTALL_PREFIX "/usr")

    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()
