# Compiling from sources

## Microsoft Windows (10, 8.1)

### Common Requirements

On Windows users should ensure that the following software is installed:

- `git` (_optional, but recommended_)
- `cmake`
- `MinGW-w64` (7.0.0 or later) with GCC toolchain 8.1.0

### Installation

1. Install `git` from <https://git-scm.com/download/win>
2. Install `cmake` from <https://cmake.org/download><br />
   Ensure that you add cmake to the $PATH system variable when following the instructions by the setup assistant.
3. Install

- _EITHER_: **MinGW-w64** from <https://sourceforge.net/projects/mingw-w64> (mingw-w64-install.exe)<br />
- _OR_: **MSVC toolchain** from Visual Studio Build Tools 2019

4. Create a new destination folder at a place of your choice
5. Open the command-line (cmd.exe) and execute `cd C:\$Path-to-your-destination-folder$\`
6. Fetch the project sourcefiles by running `git clone https://github.com/stlink-org/stlink.git`from the command-line (cmd.exe)<br />
   or download the stlink zip-sourcefolder from the Release page on GitHub

#### MSVC toolchain - minimal installation

Visual Studio IDE is not necessary, only Windows SDK & build tools are required (~3,3GB).

1. Open <https://visualstudio.microsoft.com/downloads/>
2. Navigate through menus as follows (might change overtime)

   `All downloads > Tools for Visual Studio 2019 > Build Tools for Visual Studio 2019 > Download`

3. Start downloaded executable. After Visual Studio Installer bootstraps and main window pops up, open `Individual Components` tab, and pick

- latest build tools (eg. `MSVC v142 - VS 2019 C++ x64/x86 build tools (v14.25)`)
- latest Windows SDK (eg. `Windows 10 SDK (10.0.18362.0)`)

4. After installation finishes, you can press `Launch` button in Visual Studio Installer's main menu.
   - Thus you can open `Developer Command Prompt for VS 2019`. It is `cmd.exe` instance with adjusted PATHs including eg. `msbuild`.
   - Alternatively, you can use `Developer Powershell for VS 2019` which is the same thing for `powershell.exe`. Both are available from Start menu.
   - Another option is to add `msbuild` to PATH manually. Its location should be `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin`. Then, it should be available from any `powershell.exe` or `cmd.exe` session.

### Building

#### MinGW-w64

1. Use the command-line to move to the `scripts` directory within the source-folder: `cd stlink\scripts\`
2. Execute `./mingw64-build.bat`

NOTE:<br />
Per default the build script (currently) uses `C:\Program Files\mingw-w64\x86_64-8.1.0-release-win32-sjlj-rt_v6-rev0\mingw64\bin`.<br />
When installing different toolchains make sure to update the path in the `mingw64-build.bat`.<br />
This can be achieved by opening the .bat file with a common text editor.

#### MSVC toolchain

1. In a command prompt, change the directory to the folder where the stlink files were cloned (or unzipped) to.
2. Make sure the build folder exists (`mkdir build` if not).
3. From the build folder, run cmake (`cd build; cmake ..`).

This will create a solution file `stlink.sln` in the build folder.
Now, you can build whole `stlink` suite using following command:

```
msbuild /m /p:Configuration=Release stlink.sln
```

Options:

- `/m` - compilation runs in parallel utilizing multiple cores
- `/p:Configuration=Release` - generates _Release_, optimized build.

Directory `<project_root>\build\Release` contains final executables.
(`st-util.exe` is located in `<project_root>\build\src\gdbserver\Release`).

**NOTE 1:**

Executables link against libusb.dll library. It has to be placed in the same directory as binaries or in PATH.
It can be copied from: `<project_root>\build\3rdparty\libusb-{version}\MS{arch}\dll\libusb-1.0.dll`.

**NOTE 2:**

[ST-LINK drivers](https://www.st.com/en/development-tools/stsw-link009.html) are required for `stlink` to work.

## Linux

### Common requirements

Install the following packages from your package repository:

- `git`
- `gcc` or `clang` or `mingw32-gcc` or `mingw64-gcc` (C-compiler; very likely gcc is already present)
- `build-essential` (on Debian based distros (Debian, Ubuntu))
- `cmake`
- `rpm` (on Debian based distros (Debian, Ubuntu), needed for package build with `make package`)
- `libusb-1.0`
- `libusb-1.0-0-dev` (development headers for building)
- `libgtk-3-dev` (_optional_, needed for `stlink-gui`)
- `pandoc` (_optional_, needed for generating manpages from markdown)

or execute (Debian-based systems only): `apt-get install gcc build-essential cmake libusb-1.0 libusb-1.0-0-dev libgtk-3-dev pandoc`

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
3. Run `make release` to create the _Release_ target
4. Run `make install` to full install the package with complete system integration
5. Run `make debug` to create the _Debug_ target (_optional_)<br />
   The debug target is only necessary in order to modify the sources and to run under a debugger.
6. Run `make package`to build a Debian Package. The generated packages can be found in the subdirectory `./build/dist`.

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
$ sudo cp -a config/udev/rules.d/* /etc/udev/rules.d/
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

## macOS

### Common requirements

The best and recommended way is to install a package manager for open source software,
either [homebrew](https://brew.sh) or [MacPorts](https://www.macports.org/).

Then install the following dependencies from the package repository:

- `git`
- `gcc` or `llvm` (for clang) (C-compiler)
- `cmake`
- `libusb`
- `gtk+3` or `gtk3` (_optional_, needed for `stlink-gui`)

To do this with only one simple command, type:

- for homebrew:
  - with gcc: `sudo brew install git gcc cmake libusb gtk+3` or
  - with clang: `sudo brew install git llvm cmake libusb gtk+3` or
- for MacPorts:
  - with gcc: `sudo port install git gcc10 cmake libusb gtk3` or
  - with clang: `sudo port install git llvm-10 cmake libusb gtk3`

### Installation

1. Open a new terminal window
2. Create a new destination folder at a place of your choice e.g. at `~/git`: `mkdir $HOME/git`
3. Change to this directory: `cd ~/git`
4. Fetch the project sourcefiles by running `git clone https://github.com/stlink-org/stlink.git`

### Building

1. Change into the project source directory: `cd stlink`
2. Run `make clean` to clean remnants of any previous builds.
3. Run `make release` to create the _Release_ target
4. Run `make debug` to create the _Debug_ target (_optional_)<br />
   The debug target is only necessary in order to modify the sources and to run under a debugger.

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

Author: nightwalker-87
