@echo off
echo Setting up N-Body Simulation build environment...

REM Set vcpkg path
set VCPKG_ROOT=C:\Users\deshm\vcpkg
set PATH=%VCPKG_ROOT%;%PATH%

REM Check if vcpkg is installed
if not exist "%VCPKG_ROOT%\vcpkg.exe" (
    echo ERROR: vcpkg not found at %VCPKG_ROOT%
    echo Please ensure vcpkg is properly installed
    pause
    exit /b 1
)

REM Install dependencies
echo Installing dependencies with vcpkg...
"%VCPKG_ROOT%\vcpkg.exe" install --triplet x64-windows

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:\Users\deshm\vcpkg\scripts\buildsystems\vcpkg.cmake -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: CMake configuration failed
    pause
    exit /b 1
)

echo Setup complete! Run build.bat to compile the project.
pause
