#!/bin/bash
echo "-- C compilers available"
ls -1 /usr/bin/gcc*
ls -1 /usr/bin/clang*
ls -1 /usr/bin/scan-build*
echo "----"

echo "WORK DIR:$DIR"
DIR=$PWD

if [ "$TRAVIS_JOB_NAME" == "linux-mingw-64" ]; then
    echo "--> Building for Windows (x86-64) ..."
    mkdir -p build-mingw && cd build-mingw-64
    cmake -DCMAKE_SYSTEM_NAME=Windows -DTOOLCHAIN_PREFIX=x86_64-w64-mingw32 \
          -DCMAKE_TOOLCHAIN_FILE=$PWD/../cmake/modules/set_toolchain.cmake -DCMAKE_INSTALL_PREFIX=$PWD/install $DIR
    make && rm -rf build-mingw-64 && cd -

elif [ "$TRAVIS_JOB_NAME" == "linux-mingw-32" ]; then
    echo "--> Building for Windows (i686) ..."
    mkdir -p build-mingw && cd build-mingw-32
    cmake -DCMAKE_SYSTEM_NAME=Windows -DTOOLCHAIN_PREFIX=i686-w64-mingw32 \
          -DCMAKE_TOOLCHAIN_FILE=$PWD/../cmake/modules/set_toolchain.cmake -DCMAKE_INSTALL_PREFIX=$PWD/install $DIR
    make && rm -rf build-mingw-32 && cd -

elif [ "$TRAVIS_OS_NAME" == "linux" ]; then
    sudo apt-get update -qq || true

    echo "--> Building Debug..."
    mkdir -p build/Debug && cd build/Debug
    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/install $DIR
    make && cd -

    echo "--> Building Release with package..."
    mkdir -p build/Release && cd build/Release
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/install $DIR
    make package && cd -

else # local test-build
    echo "--> Building Debug..."
    mkdir -p build/Debug && cd build/Debug
    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/install ../../
    make && cd -

    echo "--> Building Release with package..."
    mkdir -p build/Release && cd build/Release
    cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/install ../../
    make package && cd -
fi
