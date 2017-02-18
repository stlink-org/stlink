@echo off
set PATH=C:\Program Files (x86)\CMake\bin;C:\Program Files\CMake\bin;C:\Program Files\mingw-w64\x86_64-5.3.0-win32-sjlj-rt_v4-rev0\mingw64\bin;%PATH%
cmake -G "MinGW Makefiles" ..
mingw32-make
mingw32-make install DESTDIR=_install
mingw32-make package
