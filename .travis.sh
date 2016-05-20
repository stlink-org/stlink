#!/bin/bash
echo "-- C compilers available"
ls -1 /usr/bin/gcc*
ls -1 /usr/bin/clang*
ls -1 /usr/bin/scan-build*
echo "----"

if [ "$TRAVIS_OS_NAME" != "osx" ]; then
	sudo apt-get update -qq || true
	sudo apt-get install -qq -y --no-install-recommends libusb-1.0.0-dev libgtk-3-dev
else
	brew install libusb
fi

if [ "$BUILD_SYSTEM" == "cmake" ]; then
	mkdir build
	cd build
	cmake ..
	make
else
	./autogen.sh
	if [ "$TRAVIS_OS_NAME" == "osx" ]; then
		./configure
	else
		./configure --with-gtk-gui
	fi
	make
fi
