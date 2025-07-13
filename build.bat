@echo off
echo Building N-Body Simulation...

if not exist build (
    echo ERROR: Build directory not found. Run setup.bat first.
    pause
    exit /b 1
)

cd build

REM Build the project
cmake --build . --config Release --parallel

if %ERRORLEVEL% NEQ 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo Build successful!
echo Executable: build\Release\NBodySimulation.exe
cd ..
pause

