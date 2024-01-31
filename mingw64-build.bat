@echo on

mkdir build-mingw
cd build-mingw
set PATH=C:\Program Files\CMake\bin;C:\mingw-w64\x86_64-13.2.0-release-win32-seh-msvcrt-rt_v11-rev1\mingw64\bin;%PATH%
cmake -G "MinGW Makefiles" ..
mingw32-make
mingw32-make install
mingw32-make package
cd ..
