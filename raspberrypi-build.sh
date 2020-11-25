#!/bin/bash

mkdir -p build/raspberry
mkdir -p build/raspberry/inc
mkdir -p build/raspberry/bin

cat << EOF > build/raspberry/inc/version.h
#ifndef STLINK_VERSION_H_
#define STLINK_VERSION_H_

#define STLINK_VERSION       "1.6.1-116-g27d3-pi-dirty"
#define STLINK_VERSION_MAJOR 1
#define STLINK_VERSION_MINOR 6
#define STLINK_VERSION_PATCH 1

#endif // STLINK_VERSION_H_
EOF

gcc -std=gnu99 -Iinc -Isrc/stlink-lib/ -I/usr/include/libusb-1.0/ -Ibuild/raspberry/inc -DLIBUSB_API_VERSION=MINIMAL_API_VERSION -DSTLINK_HAVE_SYS_MMAN_H src/*.c src/stlink-lib/*.c src/st-trace/*.c -lusb-1.0 -o build/raspberry/bin/st-trace

