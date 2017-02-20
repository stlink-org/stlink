# Compiling

## Build from sources

* CMake (minimal v2.8.7)
* C compiler (gcc, clang, mingw)
* Libusb 1.0 (minimal v1.0.9)
* (optional) pandoc for generating manpages from markdown

Run from the root of the source directory:

```
$ make release
$ make debug
```

The debug target should only be necessary for people who want
 to modify the sources and run under a debugger.
The top level Makefile is just a handy wrapper for:

```
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make
```

You could install to a user folder e.g `$HOME`:

```
$ cd build/Release; make install DESTDIR=$HOME
```

Or system wide:

```
$ cd build/Release; sudo make install
```

## Linux

## Common requirements

* Debian based distros (debian, ubuntu)
  * `build-essential`
* `cmake`
* `libusb-1.0` (plus development headers for building, on debian based distros `libusb-1.0.0-dev` package)
* (optional) for `stlink-gui` we need libgtk-3-dev

### Fixing cannot open shared object file

When installing system-wide (`sudo make install`) the dynamic library cache needs to be updated with the command `ldconfig`.

## Permissions with udev

Make sure you install udev files which are necessary to run the tools without root
 permissions. By default most distributions don't allow access to USB devices. The
 udev rules create devices nodes and set the group of this to `stlink.

The rules are located in the `etc/udev/rules.d` directory. You will need to copy it
to /etc/udev/rules.d, and then either execute as root (or reboot your machine):

```
$ udevadm control --reload-rules
$ udevadm trigger
```

Udev will now create device node files `/dev/stlinkv2_XX`, `/dev/stlinkv1_XX`. You must
 make sure the `stlink` group exists and the user who is trying to access is added
 to this group.

### Note for STLINKv1 usage

The STLINKv1's SCSI emulation is very broken, so the best thing to do
is tell your operating system to completely ignore it.

Options (do one of these before you plug it in)

* `modprobe -r usb-storage && modprobe usb-storage quirks=483:3744:i`
* or 1. `echo "options usb-storage quirks=483:3744:i" >> /etc/modprobe.conf`
*    2. `modprobe -r usb-storage && modprobe usb-storage`
* or 1. `cp stlink_v1.modprobe.conf /etc/modprobe.d`
*    2. `modprobe -r usb-storage && modprobe usb-storage`

### Build Debian Package

To build the debian package you need the following extra packages: `debuild debhelper`.

```
$ git archive --prefix=$(git describe)/ HEAD | bzip2 --stdout > ../libstlink_$(sed -En -e "s/.*\((.*)\).*/\1/" -e "1,1 p" debian/changelog).orig.tar.bz2
$ debuild -uc -us
```

## Mac OS X

When compiling on a mac you need the following:

* A compiler toolchain (XCode)
* CMake
* Libusb 1.0

The best way is to install [homebrew](http://brew.sh) which is a package manager
 for opensource software which is missing from the Apple App Store. Then install
 the dependencies:

```
brew install libusb cmake
```

Compile as described in the first section of this document.

## Build using different directories for udev and modprobe

To put the udev or the modprobe configuration files into a different directory
during installation you can use the following cmake options:

```
$ cmake -DSTLINK_UDEV_RULES_DIR="/usr/lib/udev/rules.d" \
        -DSTLINK_MODPROBED_DIR="/usr/lib/modprobe.d" ..
```

## Windows (MinGW64) 

### Prequistes

* 7Zip
* CMake 2.8 or higher
* MinGW64 GCC toolchain (5.3.0)

### Installation

1. Install 7Zip from <http://www.7-zip.org>
2. Install CMake from <https://cmake.org/download>
3. Install MinGW64 from <https://sourceforge.net/projects/mingw-w64> (mingw-w64-install.exe)
4. Git clone or download stlink sourcefiles zip

### Building

Check and execute (in the script folder) `<source-dir>\scripts\mingw64-build.bat`

NOTE: when installing different toolchains make sure you edit the path in the `mingw64-build.bat`
      the build script uses currently `C:\Program Files\mingw-w64\x86_64-5.3.0-win32-sjlj-rt_v4-rev0\mingw64\bin`
