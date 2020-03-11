#!/bin/bash

DIR=$PWD

if [ "$TRAVIS_OS_NAME" == "linux" ]; then
	echo "WORK DIR:$DIR"
	mkdir -p $DIR/build/linux-mingw32-release
	cd $DIR/build/linux-mingw32-release
	echo "cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$PWD/_install $DIR"
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$PWD/_install $DIR 
	echo "make"
	make
fi

