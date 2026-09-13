# Compiling from sources

## Microsoft Windows - MSVC

### Common Requirements

Install the following tools:

- `git`, available on `PATH`.
- [CMake](https://cmake.org/download/), available on `PATH`: 4.2 or newer for the [Visual Studio 2026 generator](https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2018%202026.html), or 3.21 or newer for the [Visual Studio 2022 generator](https://cmake.org/cmake/help/latest/generator/Visual%20Studio%2017%202022.html).
- Visual Studio 2026 (VS 18) or Visual Studio 2022 (VS 17), including their Build Tools editions, with the **Desktop development with C++** workload, MSVC x64/x86 tools and a Windows SDK.
- [vcpkg](https://learn.microsoft.com/en-us/vcpkg/get_started/get-started-vs), to install the dependencies listed in the project's `vcpkg.json`.

### Installation

The following commands use PowerShell. Use directories you can write to; building does not require an administrator terminal.

If vcpkg is not installed, clone and bootstrap it:

```powershell
$env:VCPKG_ROOT = "$env:USERPROFILE\vcpkg"
git clone https://github.com/microsoft/vcpkg.git "$env:VCPKG_ROOT"
& "$env:VCPKG_ROOT\bootstrap-vcpkg.bat"
```

If you already have vcpkg, set `VCPKG_ROOT` to that installation instead, for example:

```powershell
$env:VCPKG_ROOT = "D:\vcpkg"
```

Set `VCPKG_ROOT` in each new terminal session, or save it as a user environment variable.

Fetch the project source files and enter the repository root:

```powershell
git clone https://github.com/stlink-org/stlink.git
cd stlink
```

### Building

From the repository root, run:

```powershell
.\gen_binaries_msvc.bat
```

The script uses `vswhere`, supplied by the Visual Studio Installer, to detect VS 18 with the C++ tools installed. It prefers VS 18; if none is found, it tries VS 17. It configures x64 and builds Release.
Each version uses a separate build directory so that switching versions does not conflict with a cached CMake generator:

| Visual Studio | Build directory |
| --- | --- |
| VS 18 (2026) | `build/msvc-vcpkg-vs18` |
| VS 17 (2022) | `build/msvc-vcpkg-vs17` |

During configuration, vcpkg installs `libusb` and `pthreads` from `vcpkg.json` into the selected build directory's `vcpkg_installed` subdirectory.
CMake selects PThreads4W for MSVC and links the matching Release or Debug dependency libraries. The Windows compatibility headers supply the required POSIX types.

The equivalent manual commands for VS 18 are below. For VS 17, replace the generator with `Visual Studio 17 2022` and use `build/msvc-vcpkg-vs17` in the commands below.

```powershell
cmake -S . -B build/msvc-vcpkg-vs18 -G "Visual Studio 18 2026" -A x64 "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build/msvc-vcpkg-vs18 --config Release
```

Visual Studio uses `--config` to select the build configuration. To build Debug using the same configured tree:

```powershell
cmake --build build/msvc-vcpkg-vs18 --config Debug
```

The executables are in the selected build directory's `bin/Release` or `bin/Debug` subdirectory.
With vcpkg's default settings, the required dependency DLLs are copied beside them during the build.
For example, check the Release build with:

```powershell
.\build\msvc-vcpkg-vs18\bin\Release\st-info.exe --version
```

If an existing build directory was configured with a different generator, architecture or toolchain, use a new build directory for the manual commands.

**NOTE:**

1. [ST-LINK drivers](https://www.st.com/en/development-tools/stsw-link009.html) are required for programmers to work with `stlink`.
2. Package generation for MSVC is not yet implemented/tested.

## Linux

### Common requirements

Install the following packages from your package repository:

- `git`
- `gcc` and `g++` or `clang` (C-compiler)
- `make` (Build tool)
- `build-essential` (_recommended_, on Debian based distros, contains `gcc`, `g++`, `libc6-dev`, `make`)
- `cmake` (Software development tool)
- `libusb-1.0-0` and `libusb-1.0-0-dev` (libusb and related development headers)
- `libgtk-3-dev` (_optional_, needed for `stlink-gui`)
- `rpm` (on Debian based distros, needed for package build with `make package`)
- `pandoc` (_optional_, needed for generating manpages from markdown)

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

- `mingw-w64`, `autotools-dev` and `libtool`

After following the steps for installation above, proceed with from the build dircetory itself:

```sh
$ sudo sh ./gen_binaries_win.sh
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
