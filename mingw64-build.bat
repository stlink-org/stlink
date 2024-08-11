@echo on

mkdir build-mingw
cd build-mingw
set PATH=C:\Program Files\CMake\bin;C:\msys64\ucrt64\bin;C:\msys64\mingw64\bin;%PATH%
cmake -G "MinGW Makefiles" ..
mingw32-make
mingw32-make install
mingw32-make package
cd ..
