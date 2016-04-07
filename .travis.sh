#!/bin/bash
if [ "$TRAVIS_OS_NAME" != "osx" ]; then
	sudo apt-get update -qq || true
	sudo apt-get install -qq -y --no-install-recommends libusb-1.0.0-dev
else
	brew install libusb
fi

mkdir build
cd build
cmake ..
make
