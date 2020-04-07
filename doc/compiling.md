# Compiling from sources

## Microsoft Windows (10, 8.1)
### Common Requirements

On Windows users should ensure that the following software is installed:

* `7zip`
* `git`
* `cmake` (3.17.0 or later)
* `MinGW-w64` (7.0.0 or later) with GCC toolchain 8.1.0


### Installation

1. Install `7zip` from <https://www.7-zip.org>
2. Install `git` from <https://git-scm.com/download/win>
3. Install `cmake` from <https://cmake.org/download><br />
   Ensure that you add cmake to the $PATH system variable when following the instructions by the setup assistant.
4. Install
  - _EITHER_: **MinGW-w64** from <https://sourceforge.net/projects/mingw-w64> (mingw-w64-install.exe)<br />
  - _OR_:     **Visual Studio 2017 CE** (other versions will likely work as well, but are untested; the Community edition is free for open source
  development)
5. Create a new destination folder at a place of your choice
6. Open the command-line (cmd.exe) and execute `cd C:\$Path-to-your-destination-folder$\`
7. Fetch the project sourcefiles by running `git clone https://github.com/texane/stlink.git`from the command-line (cmd.exe)<br />
  or download the stlink zip-sourcefolder from the Release page on GitHub


### Building
#### MinGW-w64

1. Use the command-line to move to the `scripts` directory within the source-folder: `cd stlink\scripts\`
2. Execute `./mingw64-build.bat`

NOTE:<br />
Per default the build script (currently) uses `C:\Program Files\mingw-w64\x86_64-8.1.0-release-win32-sjlj-rt_v6-rev0\mingw64\bin`.<br />
When installing different toolchains make sure to update the path in the `mingw64-build.bat`.<br />
This can be achieved by opening the .bat file with a common text editor.


#### Visual Studio (32 bit)

1. In a command prompt, change the directory to the folder where the stlink files were cloned (or unzipped) to.
2. Make sure the build folder exists (`mkdir build` if not).
3. From the build folder, run cmake (`cd build; cmake ..`).

This will create a solution (stlink.sln) in the build folder. Open it in Visual Studio, select the Solution Configuration (Debug or
Release) and build the solution normally (F7).

NOTE:<br />
This solution will link to the dll version of libusb-1.0.y<br />
To debug or run the executable, the dll version of libusb-1.0 must be either on the path, or in the same folder as the executable.<br />
It can be copied from here: `build\3rdparty\libusb-1.0.21\MS32\dll\libusb-1.0.dll`.

## Linux
### Common requirements

Install the following packages from your package repository:

* `gcc` or `clang` or `mingw32-gcc` or `mingw64-gcc` (C-compiler; very likely gcc is already present)
* `build-essential` (on Debian based distros (debian, ubuntu))
* `cmake` (3.4.2 or later, use the latest version available from the repository)
* `libusb-1.0`
* `libusb-1.0-0-dev` (development headers for building)
* `libgtk-3-dev` (_optional_, needed for `stlink-gui`)
* `pandoc` (_optional_, needed for generating manpages from markdown)

or execute (Debian-based systems only): `apt-get install gcc build-essential cmake libusb-1.0 libusb-1.0-0-dev libgtk-3-dev pandoc`

(Replace gcc with the intended C-compiler if necessary or leave out any optional package not needed.)


### Installation

1. Open a new terminal console
2. Create a new destination folder at a place of your choice e.g. at `~/git`: `mkdir $HOME/git`
3. Change to this directory: `cd ~/git`
4. Fetch the project sourcefiles by running `git clone https://github.com/texane/stlink.git`


### Building

1. Change into the project source directory: `cd stlink`
2. Run `make release` to create the _Release_ target
3. Run `make debug` to create the _Debug_ target (_optional_)<br />
   The debug target is only necessary in order to modify the sources and to run under a debugger.

The top level Makefile is just a handy wrapper for:

##### MinGW64:

```sh
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw64.cmake -S ..
$ make
```

##### MinGW32:

```sh
$ mkdir build && cd build
$ cmake -DCMAKE_BUILD_TYPE=release -DCMAKE_TOOLCHAIN_FILE=./cmake/linux-mingw32.cmake -S ..
$ make
```

As an alternative you may also install
- to a user folder e.g `$HOME` with `cd build/Release && make install DESTDIR=$HOME`
- or system wide with `cd build/Release && sudo make install`.

When installing system-wide, the dynamic library cache needs to be updated with the command `ldconfig`.


### Build a Debian Package

To build the debian package you need the following extra packages: `devscripts debhelper`.

```sh
$ git archive --prefix=$(git describe)/ HEAD | bzip2 --stdout > ../libstlink_$(sed -En -e "s/.*\((.*)\).*/\1/" -e "1,1 p" debian/changelog).orig.tar.bz2
$ debuild -uc -us
```


### Set permissions with udev

By default most distributions don't allow access to USB devices.
Therefore make sure you install udev files which are necessary to run the tools without root permissions.
udev rules create devices nodes and set the group of these to `stlink`.

The rules are located in the subdirectory `etc/udev/rules.d` within the sourcefolder.
Copy them to the directory path `/etc/udev/rules.d` and subsequently reload the udev rules:

```sh
$ cp etc/udev/rules.d /etc/udev/rules.d
$ udevadm control --reload-rules
$ udevadm trigger
```

Udev will now create device node files `/dev/stlinkv2_XX`, `/dev/stlinkv1_XX`.
You need to ensure that the group `stlink` exists and the user who is trying to access these devices is a member of this group.


### Note on the use of STLink-v1 programmers:

At the time of writing the STLink-v1 has mostly been replaced with the newer generation STLink-v2 programmers and thus is only rarely used.
As there are some caveats as well, we recommend to use the STLink-v2 programmers if possible.

To be more precise, the STLINKv1's SCSI emulation is somehow broken, so the best advice possibly is to tell your operating system to completely ignore it.

Choose on of the following options _before_ connecting the device to your computer:

* `modprobe -r usb-storage && modprobe usb-storage quirks=483:3744:i`
* _OR_
    1. `echo "options usb-storage quirks=483:3744:i" >> /etc/modprobe.conf`
    2. `modprobe -r usb-storage && modprobe usb-storage`
* _OR_
    1. `cp stlink_v1.modprobe.conf /etc/modprobe.d`
    2. `modprobe -r usb-storage && modprobe usb-storage`


## macOS
### Common requirements

The best way is to install a package manager for open source software,
either [homebrew](https://brew.sh) or [MacPorts](https://www.macports.org/).

Then install the following dependencies from the package repository:

* `git`
* `cmake`
* `libusb`

To do this with only one simple command, type:

* for homebrew: `sudo brew install git cmake libusb` or
* for MacPorts:`sudo port install git cmake libusb`

Additionally we recommend to install Xcode which delivers the necessary C-compiler toolchain Clang (LLVM).


### Installation

1. Open a new terminal window
2. Create a new destination folder at a place of your choice e.g. at `~/git`: `mkdir $HOME/git`
3. Change to this directory: `cd ~/git`
4. Fetch the project sourcefiles by running `git clone https://github.com/texane/stlink.git`


### Building

1. Change into the project source directory: `cd stlink`
2. Run `make release` to create the _Release_ target
3. Run `make debug` to create the _Debug_ target (_optional_)<br />
   The debug target is only necessary in order to modify the sources and to run under a debugger.


## Build using a different directory for shared libs

To put the compiled shared libs into a different directory during installation,
you can use the cmake option `cmake -DLIB_INSTALL_DIR:PATH="/usr/lib64" ..`.


Author: nightwalker-87
