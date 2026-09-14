@echo off

echo Building stlink binaries for MSVC with vcpkg dependencies
setlocal

set "STLINK_GENERATOR=Visual Studio 17 2022"
set "STLINK_BUILD_DIR=build/msvc-vcpkg-vs17"
set "STLINK_CMAKE_MIN=3.21"
set "STLINK_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%STLINK_VSWHERE%" (
    for /f "delims=" %%I in ('call "%STLINK_VSWHERE%" -latest ^
        -products * -version "[17.0,18.0,19.0)" ^
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 ^
        -property installationPath') do (
        set "STLINK_GENERATOR=Visual Studio 18 2026"
        set "STLINK_BUILD_DIR=build/msvc-vcpkg-vs18"
        set "STLINK_CMAKE_MIN=4.2"
    )
)

for /f "tokens=3" %%V in ('cmake --version ^| findstr /b /c:"cmake version"') do set "STLINK_CMAKE_VERSION=%%V"
if not defined STLINK_CMAKE_VERSION (
    echo ERROR: could not determine the installed CMake version. Is cmake on PATH?
    exit /b 1
)

call :version_less "%STLINK_CMAKE_VERSION%" "%STLINK_CMAKE_MIN%" STLINK_CMAKE_TOO_OLD
if "%STLINK_CMAKE_TOO_OLD%"=="1" (
    echo ERROR: %STLINK_GENERATOR% requires CMake %STLINK_CMAKE_MIN%+, found %STLINK_CMAKE_VERSION%.
    echo Install a matching CMake version, or install a Visual Studio version
    echo supported by your current CMake - see doc/compiling.md.
    exit /b 1
)

echo Using %STLINK_GENERATOR%
cmake -S . -B "%STLINK_BUILD_DIR%" -G "%STLINK_GENERATOR%" -A x64 ^
    "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" || goto :error
cmake --build "%STLINK_BUILD_DIR%" --config Release || goto :error

:ok
exit /b 0

:error
exit /b %errorlevel%

:version_less
setlocal
for /f "tokens=1,2 delims=." %%A in ("%~1") do (set "A_MAJ=%%A" & set "A_MIN=%%B")
for /f "tokens=1,2 delims=." %%A in ("%~2") do (set "B_MAJ=%%A" & set "B_MIN=%%B")
set "RESULT=0"
if %A_MAJ% LSS %B_MAJ% set "RESULT=1"
if %A_MAJ% EQU %B_MAJ% if %A_MIN% LSS %B_MIN% set "RESULT=1"
endlocal & set "%~3=%RESULT%"
exit /b 0
