###
# MinGW64 build helper for windows
# * Sets the correct PATH to compiler and tools
# * Generates MinGW Makefile with CMake
# * Build binaries with mingw32-make
#
# Read documentation about this file in doc/build_mingw.md
###
@echo off
set PATH=C:\Program Files (x86)\CMake\bin;C:\Program Files\CMake\bin;C:\Program Files\mingw-w64\x86_64-5.3.0-win32-sjlj-rt_v4-rev0\mingw64\bin;%PATH%
cmake -G "MinGW Makefiles" ..
mingw32-make
