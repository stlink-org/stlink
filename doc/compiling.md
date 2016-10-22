# Compiling

## Build from sources



* CMake
* C compiler (gcc, clang, mingw)
* Libusb 1.0
* (optional) pandoc for generating manpages from markdown

```
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make
```

## Linux

## Common requirements

* Debian based distros (debian, ubuntu)
  * `build-essential`
* `cmake`
* `libusb-1.0` (plus development headers for building, on debian based distros `libusb-1.0.0-dev` package)
* (optional) for `stlink-gui` we need libgtk-3-dev

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
