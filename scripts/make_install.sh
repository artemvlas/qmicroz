#! /bin/bash

# [Arch Linux (KDE)]
# Required packages: make, cmake
# sudo pacman -S base-devel cmake

set -x
set -e

TEMP_BASE=/tmp

REPO_ROOT=$(readlink -f $(dirname $(dirname $0)))
BUILD_DIR=$(mktemp -d -p "$TEMP_BASE" qmicroz-build-XXXXXX)

cleanup () {
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
    fi
}
trap cleanup EXIT

pushd "$BUILD_DIR"

# make the project
cmake "$REPO_ROOT" -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON

# build
make -j2

# install [/usr/lib/libqmicroz.so, /usr/include/qmicroz.h]
sudo make install

echo "All done..."
