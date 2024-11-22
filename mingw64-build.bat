@echo off
set MAKE_OPTIONS=%1

if not "%MAKE_OPTIONS%"=="" (
    echo Using custom make options: %MAKE_OPTIONS%
)

@echo on
mkdir build-mingw
cd build-mingw
set PATH=C:\Program Files\CMake\bin;C:\msys64\ucrt64\bin;C:\msys64\mingw64\bin;%PATH%
cmake -G "MinGW Makefiles" ..
mingw32-make %MAKE_OPTIONS%
mingw32-make install %MAKE_OPTIONS%
mingw32-make package %MAKE_OPTIONS%
cd ..
