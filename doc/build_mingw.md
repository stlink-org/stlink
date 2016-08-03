Building with MinGW under Windows
=================================

## Prequistes

* 7Zip
* CMake 2.8 or higher
* MinGW64 GCC toolchain (5.3.0)

## Installation

1. Install 7Zip from http://www.7-zip.org
2. Install CMake from https://cmake.org/download
3. Install MinGW64 from https://sourceforge.net/projects/mingw-w64 (mingw-w64-install.exe)
4. Git clone or download stlink sourcefiles zip
5. Create build folder in source

# Building

Check and execute `<source-dir>\scripts\mingw64-build.bat`

NOTE: when installing different toolchains make sure you edit the path in the `mingw64-build.bat`
      the build script uses currently `C:\Program Files\mingw-w64\x86_64-5.3.0-win32-sjlj-rt_v4-rev0\mingw64\bin`
