#############
## QMicroz ##
#############
cmake_minimum_required(VERSION 3.16)

project(qmicroz VERSION 0.7)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Enable building of unit tests" ON)
option(INSTALL_FILES "Enable installation" ON)

# find Qt packages
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

# directories
include_directories("src" "miniz")

# sources
add_library(qmicroz
  src/qmicroz.h
  src/qmicroz.cpp
  miniz/miniz.h
  miniz/miniz.c
)

# target
target_link_libraries(qmicroz PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(qmicroz PUBLIC src)

if(BUILD_SHARED_LIBS)
  target_compile_definitions(qmicroz PRIVATE MAKE_SHARED)
  message("Make SHARED")
else()
  message("Make STATIC")
endif()

# unit test
if(BUILD_TESTS)
  enable_testing()
  find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Test)
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)

  add_executable(test_qmicroz src/test_qmicroz.cpp)
  add_test(NAME test_qmicroz COMMAND test_qmicroz)

  target_link_libraries(test_qmicroz PRIVATE Qt${QT_VERSION_MAJOR}::Test qmicroz)
endif(BUILD_TESTS)

# install [/usr/lib/libqmicroz.so, /usr/include/qmicroz.h]
if(INSTALL_FILES AND ${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(CMAKE_INSTALL_PREFIX "/usr")

    include(GNUInstallDirs)
    install(TARGETS ${PROJECT_NAME}
        BUNDLE DESTINATION .
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    # install header
    install(FILES src/qmicroz.h DESTINATION /usr/include)
endif()
