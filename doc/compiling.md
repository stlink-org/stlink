# Compiling

## Build from sources

* CMake
* C compiler (gcc, clang, mingw)
* Libusb 1.0
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
$ cd release; make install DESTDIR=$HOME
```

Or system wide:

```
$ cd release; sudo make install
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
To build debian package you need debuild.

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
