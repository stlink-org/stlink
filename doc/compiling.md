# Compiling from sources

## Microsoft Windows - MSVC (11)

### Common Requirements

On Windows users should ensure that the following software is installed:

- `git` (Required for building LibUSB if missing)
- `7zip` (_optional_)
- `cmake`
- `MSVC` Compiler (Tested with Visual Studio 2022 and Build Tools for Visual Studio 2022)

### Installation

1. Install `Build Tools for Visual Studio` from <https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022>
2. Install `cmake` from <https://cmake.org/download/#latest> --> Binary distributions --> Windows x64 Installer<br />
   Ensure that you add cmake to the $PATH system variable when following the instructions by the setup assistant.
   Follow the installation instructions on the website.
3. Fetch the project source files by running `git clone https://github.com/stlink-org/stlink.git` from the command-line (`cmd.exe`/`powershell.exe`)<br />
   or download and extract (`7zip`) the latest stlink `.zip` release from the Release page on GitHub.

### Building

1. Open the command-line (`cmd.exe`/`powershell.exe`) with administrator privileges
2. Move to the `stlink` directory with `cd C:\$Path-to-your-stlink-folder$\`
3. Create a new `build` subdirectory and move into it with `cd .\build`.
4. Configure the project, using the following command: `cmake -G "Visual Studio 17 2022" .. -DCMAKE_BUILD_TYPE="Release"`
5. Build the project, using the following command: `cmake --build . --target ALL_BUILD`
6. Install the project, using the following command: `cmake --build . --target INSTALL`
7. Add the `bin` folder of the installation path (`C:\Program Files (x86)\stlink\bin`) to the `PATH` environment variables:
   1. Run `SystemPropertiesAdvanced.exe`
   2. press on `Environment Variables` button
   3. On `System Variables` list, find and select `Path` variable
   4. Press `Edit..` button bellow the list
   5. On the new Window, press `New` button
   6. On the new row, type the `bin` path of your installation (`C:\Program Files (x86)\stlink\bin`)
   7. Press `OK` button to all three windows to save your changes

**NOTE:**

1. [ST-LINK drivers](https://www.st.com/en/development-tools/stsw-link009.html) are required for programmers to work with `stlink`.
2. Package generation for MSVC is not yet implemented/tested.

## Linux

### Common requirements

Install the following packages from your package repository:

- `git`
- `gcc` or `clang` or `mingw32-gcc` or `mingw64-gcc` (C-compiler; very likely gcc is already present)
- `build-essential` (on Debian based distros (Debian, Ubuntu))
- `cmake`
- `rpm` (on Debian based distros (Debian, Ubuntu), needed for package build with `make package`)
- `libusb-1.0-0`
- `libusb-1.0-0-dev` (development headers for building)
- `libgtk-3-dev` (_optional_, needed for `stlink-gui`)
- `pandoc` (_optional_, needed for generating manpages from markdown)

or execute (Debian-based systems only): `apt-get install gcc build-essential cmake rpm libusb-1.0-0 libusb-1.0-0-dev libgtk-4-dev pandoc`

(Replace gcc with the intended C-compiler if necessary or leave out any optional package not needed.)

### Installation

1. Open a new terminal console
2. Create a new destination folder at a place of your choice e.g. at `~/git`: `mkdir $HOME/git`
3. Change to this directory: `cd ~/git`
4. Fetch the project sourcefiles by running `git clone https://github.com/stlink-org/stlink.git`

### Building

#### Installation:

1. Change into the project source directory: `cd stlink`
2. Run `make clean` -- required by some linux variants.
3. Run `make release` to create the _Release_ target.
4. Run `make install` to full install the package with complete system integration. This might require sudo permissions.
5. Run `make debug` to create the _Debug_ target (_optional_)<br />
   The debug target is only necessary in order to modify the sources and to run under a debugger.
6. Run `make package`to build a Debian Package. The generated packages can be found in the subdirectory `./build/Release/dist`.
7. Installing system-wide (`sudo make install`) requires the dynamic library cache to be updated with `sudo ldconfig` afterwards.

As an option you may also install to an individual user-defined folder e.g `$HOME` with `make install DESTDIR=$HOME`.

#### Removal:

1. Run `make uninstall` to perform a clean uninstall of the package from the system.
2. Run `make clean` to clean the build-folder within the project source and remove all compiled and linked files and libraries.

### Cross-Building for Windows

Install the following packages from your package repository:

- `mingw-w64`
- `mingw-w64-common`
- `mingw-w64-i686-dev`
- `mingw-w64-x86-64-dev`

After following the steps for installation above, proceed with from the build dircetory itself:

```sh
$ sudo sh ./cmake/packaging/windows/generate_binaries.sh
```

The generated zip-packages can be found in the subdirectory `./build/dist`.

### Set device access permissions and the role of udev

By default most distributions don't allow access to USB devices.
In this context udev rules, which create devices nodes, are necessary to run the tools without root permissions.
To achieve this you need to ensure that the group `plugdev` exists and the user who is trying to access these devices is a member of this group.

Within the sourcefolder of the project, these rules are located in the subdirectory `config/udev/rules.d` and are automatically installed along with `sudo make install` on linux.
Afterwards it may be necessary to reload the udev rules:

```sh
$ sudo cp -a config/udev/rules.d/* /lib/udev/rules.d/
$ sudo udevadm control --reload-rules
$ sudo udevadm trigger
```

udev will now create device node files, e.g. `/dev/stlinkv3_XX`, `/dev/stlinkv2_XX`, `/dev/stlinkv1_XX`.

### Special note on the use of STLink/V1 programmers (legacy):

As the STLINKV1's SCSI emulation is somehow broken, the best advice possibly is to tell your operating system to completely ignore it.<br />
Choose one of the following options _before_ connecting the device to your computer:

- `modprobe -r usb-storage && modprobe usb-storage quirks=483:3744:i`
- _OR_
  1. `echo "options usb-storage quirks=483:3744:i" >> /etc/modprobe.conf`
  2. `modprobe -r usb-storage && modprobe usb-storage`
- _OR_
  1. `cp stlink_v1.modprobe.conf /etc/modprobe.d`
  2. `modprobe -r usb-storage && modprobe usb-storage`

## Build options

### Build using a different directory for shared libs

To put the compiled shared libs into a different directory during installation,
you can use the cmake option `cmake -DLIB_INSTALL_DIR:PATH="/usr/lib64" ..`.

### Standard installation directories

The cmake build system of this toolset includes `GNUInstallDirs` to define GNU standard installation directories.
This module provides install directory variables as defined by the GNU Coding Standards.

Below are the preset default cmake options, which apply if none of these options are redefined:

- `-DCMAKE_INSTALL_SYSCONFDIR=/etc`
- `-DCMAKE_INSTALL_PREFIX=/usr/local`

Please refer to the related [cmake documentation](https://cmake.org/cmake/help/latest/variable/CMAKE_INSTALL_PREFIX.html) for details.

Author: nightwalker-87
