@echo off
echo Running N-Body Simulation...

if not exist build\Release\NBodySimulation.exe (
    echo ERROR: Executable not found. Run build.bat first.
    pause
    exit /b 1
)

cd build
Release\NBodySimulation.exe
