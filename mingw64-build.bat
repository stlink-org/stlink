@echo on

mkdir build-mingw
cd build-mingw
set PATH=C:\Program Files (x86)\CMake\bin;C:\Program Files\CMake\bin;C:\Program Files\mingw-w64\x86_64-8.1.0-win32-sjlj-rt_v6-rev0\mingw64\bin;%PATH%
cmake -G "MinGW Makefiles" ..
mingw32-make
mingw32-make install DESTDIR=install
mingw32-make package
