#!/usr/bin/env bash

sudo apt-get -y update
sudo apt-get -y install libusb-1.0 libusb-1.0-0-dev libgtk-3-dev
make clean
make release -j$(nproc)
