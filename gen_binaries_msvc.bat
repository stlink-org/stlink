@echo off

echo Building stlink binaries for MSVC with vcpkg dependencies
setlocal

set "STLINK_GENERATOR=Visual Studio 17 2022"
set "STLINK_BUILD_DIR=build/msvc-vcpkg-vs17"
set "STLINK_VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if exist "%STLINK_VSWHERE%" (
    for /f "delims=" %%I in ('call "%STLINK_VSWHERE%" -latest -products * -version "[18.0,19.0)" -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath') do (
        set "STLINK_GENERATOR=Visual Studio 18 2026"
        set "STLINK_BUILD_DIR=build/msvc-vcpkg-vs18"
    )
)

echo Using %STLINK_GENERATOR%
cmake -S . -B "%STLINK_BUILD_DIR%" -G "%STLINK_GENERATOR%" -A x64 "-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" || goto :error
cmake --build "%STLINK_BUILD_DIR%" --config Release || goto :error

:ok
exit /b 0

:error
exit /b %errorlevel%
