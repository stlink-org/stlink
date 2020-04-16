#!/bin/bash
echo "-- C compilers available"
ls -1 /usr/bin/gcc*
ls -1 /usr/bin/clang*
ls -1 /usr/bin/scan-build*
echo "----"

echo "WORK DIR:$DIR"
DIR=$PWD

if [ "$TRAVIS_OS_NAME" == "linux" ]; then
	sudo apt-get update -qq || true
	sudo apt-get install -qq -y --no-install-recommends libgtk-3-dev

	echo "--> Building Debug..."
	mkdir -p build/Debug && cd build/Debug
	echo "-DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/_install"
	cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/_install ../../
	make && make package && cd -

	echo "--> Building Release..."
	mkdir -p build/Release && cd build/Release
	echo "-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/_install"
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/_install ../../
	make && make package && cd -

#	echo "--> Building Binary..."
#	mkdir -p build/Binary && cd build/Binary
#	cho "-DCMAKE_BUILD_TYPE=Binary -DCMAKE_INSTALL_PREFIX=$PWD/_install"
#	cmake -DCMAKE_BUILD_TYPE=Binary -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$PWD/_install ../../
#	make && make package && cd -
else [ "$TRAVIS_OS_NAME" == "osx" ];
	brew install libusb

	echo "--> Building Debug..."
	mkdir -p build/Debug && cd build/Debug
	echo "-DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/_install"
	cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=$PWD/_install ../../
	make && make package && cd -

	echo "--> Building Release..."
	mkdir -p build/Release && cd build/Release
	echo "-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/_install"
	cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/_install ../../
	make && make package && cd -
fi
