# N-Body Simulation - C++ Build Guide

## Project Status ✅

**Implementation Complete:**
- ✅ Core Application framework with GLFW/OpenGL
- ✅ Physics Engine with Barnes-Hut algorithm
- ✅ High-performance OpenGL renderer
- ✅ ImGui-based user interface
- ✅ Modular architecture with proper headers
- ✅ CMake build system with vcpkg integration
- ✅ Batch scripts for easy building

**Features Implemented:**
- ✅ Real-time N-body gravitational simulation
- ✅ Barnes-Hut tree algorithm for O(N log N) performance
- ✅ Instanced rendering for thousands of bodies
- ✅ Interactive UI with simulation controls
- ✅ Multiple integration presets (Solar System, Binary, Galaxy, etc.)
- ✅ Collision detection and response
- ✅ Energy conservation monitoring
- ✅ Performance statistics and profiling

## Prerequisites

1. **Visual Studio 2019/2022** or **MinGW-w64** (for GCC)
2. **CMake 3.16+**
3. **vcpkg** package manager
4. **Git** (for vcpkg)

## Installation Steps

### 1. Install vcpkg (if not already installed)

```bash
# Clone vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# Bootstrap vcpkg
.\bootstrap-vcpkg.bat

# Add to PATH or set VCPKG_ROOT environment variable
set VCPKG_ROOT=C:\vcpkg
```

### 2. Navigate to Project Directory

```bash
cd "d:\nbodyprob\N-body problem\cpp-version"
```

### 3. Run Setup Script

```bash
setup.bat
```

This will:
- Install dependencies (GLFW, GLEW, GLM, ImGui)
- Configure CMake with vcpkg toolchain
- Create build directory

### 4. Build the Project

```bash
build.bat
```

This will:
- Compile all source files
- Link libraries
- Create executable in `build\Release\`

### 5. Run the Simulation

```bash
run.bat
```

Or directly:
```bash
cd build
Release\NBodySimulation.exe
```

## Manual Build (Alternative)

If the batch scripts don't work:

```bash
# Install dependencies
vcpkg install glfw3 glew glm imgui[opengl-binding,glfw-binding] --triplet x64-windows

# Create build directory
mkdir build
cd build

# Configure with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release --parallel
```

## Usage

### Controls
- **Left Click**: Add new body (or select existing body)
- **Right Click**: Delete body under cursor
- **Mouse Wheel**: Zoom in/out
- **Middle Mouse/Ctrl+Left**: Pan camera
- **Space**: Play/Pause simulation
- **R**: Reset simulation
- **C**: Clear all bodies

### UI Panels
- **Controls**: Simulation parameters, body creation settings
- **Statistics**: Performance metrics, energy conservation
- **Body Properties**: Edit selected body parameters
- **Debug Info**: Camera and technical information

### Presets
- **Solar System**: Realistic planetary simulation
- **Binary System**: Two stars orbiting each other
- **Galaxy**: Spiral galaxy formation
- **Random Cluster**: Randomly distributed bodies

## Performance Expectations

- **Direct N²**: Up to 1,000 bodies at 60 FPS
- **Barnes-Hut**: Up to 100,000 bodies at 60 FPS
- **GPU Acceleration**: Future implementation for 1M+ bodies

## Troubleshooting

### Common Issues

1. **vcpkg not found**
   - Ensure vcpkg is in PATH or VCPKG_ROOT is set
   - Run `vcpkg integrate install`

2. **CMake configuration fails**
   - Check that all dependencies are installed
   - Verify CMake version (3.16+)
   - Try deleting build directory and re-running setup

3. **Compilation errors**
   - Ensure Visual Studio 2019+ or GCC 7+ is installed
   - Check that all header files are present
   - Try building in Debug mode for more information

4. **Runtime crashes**
   - Ensure graphics drivers support OpenGL 3.3+
   - Check that shaders directory is copied to build folder
   - Run from build directory to ensure relative paths work

### OpenGL Requirements
- **Minimum**: OpenGL 3.3
- **Recommended**: OpenGL 4.3+ for compute shaders (future GPU acceleration)

## Architecture Overview

```
cpp-version/
├── include/          # Header files (.h)
│   ├── core/        # Core classes (Application, Body)
│   ├── physics/     # Physics engine (PhysicsEngine, BarnesHut)
│   ├── rendering/   # OpenGL renderer (Renderer, Shader)
│   └── ui/          # ImGui interface (UIManager)
├── src/             # Implementation files (.cpp)
│   ├── core/        # Core implementations
│   ├── physics/     # Physics implementations
│   ├── rendering/   # Rendering implementations
│   └── ui/          # UI implementations
├── shaders/         # GLSL shaders (.vert, .frag)
├── assets/          # Textures and resources
└── build/           # Generated build files
```

## Release Build

For distribution:

1. Build in Release mode (default)
2. Copy necessary DLLs to executable directory
3. Include shaders and assets folders
4. Test on target machines without development tools

## Future Enhancements

- **GPU Compute Shaders**: Parallel force calculations on GPU
- **Multi-threading**: CPU parallelization with OpenMP
- **Advanced Integrators**: Runge-Kutta, Leapfrog variants
- **File I/O**: Save/load simulation states
- **3D Rendering**: Extension to 3D space
- **Networking**: Distributed simulations

---

**Project Complete!** 🚀

The C++ version provides significantly better performance than the JavaScript implementation while maintaining all interactive features and adding professional-grade UI and rendering capabilities.
