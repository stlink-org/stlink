#!/bin/bash
echo "-- C compilers available"
ls -1 /usr/bin/gcc*
ls -1 /usr/bin/clang*
ls -1 /usr/bin/scan-build*
echo "----"

if [ "$TRAVIS_OS_NAME" == "linux" ]; then
	sudo apt-get update -qq || true
	sudo apt-get install -qq -y --no-install-recommends libusb-1.0.0-dev libgtk-3-dev
else
	brew install libusb
fi

echo "=== Building Debug"
mkdir -p build/Debug && cd build/Debug && cmake -DCMAKE_BUILD_TYPE=Debug ../../ && make && make package && cd -

echo "=== Building Release"
mkdir -p build/Release && cd build/Release && cmake -DCMAKE_BUILD_TYPE=Release ../../ && make && make package && cd -
