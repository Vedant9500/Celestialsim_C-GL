# N-Body Simulation - C++ + OpenGL Version

A high-performance N-body gravitational simulation written in modern C++ with OpenGL for rendering.

## Features

- **High Performance**: Optimized C++ with SIMD instructions and OpenGL GPU acceleration
- **Barnes-Hut Algorithm**: O(N log N) complexity for large-scale simulations
- **GPU Compute Shaders**: Parallel force calculations on GPU
- **Interactive Interface**: Real-time parameter adjustment with ImGui
- **Collision Detection**: Solid-body physics with elastic collisions
- **Visual Effects**: Particle trails, force visualization, and dynamic lighting

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- OpenGL 4.3+ compatible graphics card
- CMake 3.16+

### Dependencies

- **GLFW**: Window management and input
- **GLEW**: OpenGL extension loading
- **GLM**: Mathematics library
- **ImGui**: Immediate mode GUI
- **OpenMP**: (Optional) CPU parallelization

## Building

### Using vcpkg (Recommended)

```bash
# Install vcpkg dependencies
vcpkg install glfw3 glew glm imgui[opengl-binding,glfw-binding]

# Build with CMake
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path-to-vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

### Manual Build

```bash
# Install dependencies manually, then:
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

## Usage

```bash
./NBodySimulation
```

### Controls

- **Mouse**: Click and drag to pan the view
- **Mouse Wheel**: Zoom in/out
- **Left Click**: Add new body (hold Shift for orbit mode)
- **Right Click**: Select body for editing
- **Space**: Play/Pause simulation
- **R**: Reset simulation
- **C**: Clear all bodies

### Configuration

The simulation supports various presets and can be configured through the GUI:

- **Solar System**: Realistic planetary simulation
- **Galaxy**: Spiral galaxy formation
- **Binary Stars**: Binary star systems
- **Clusters**: Globular clusters

## Performance

- **CPU**: Up to 100,000 bodies with Barnes-Hut algorithm
- **GPU**: Up to 1,000,000 bodies with compute shaders
- **Memory**: Efficient Structure-of-Arrays layout
- **Threading**: OpenMP parallelization for CPU calculations

## Architecture

```
├── include/           # Header files
│   ├── core/         # Core simulation classes
│   ├── physics/      # Physics calculations
│   ├── rendering/    # OpenGL rendering
│   └── ui/          # User interface
├── src/              # Source files
│   ├── core/        # Core implementation
│   ├── physics/     # Physics implementation
│   ├── rendering/   # Rendering implementation
│   └── ui/         # UI implementation
├── shaders/         # GLSL shaders
└── assets/         # Textures and resources
```

## License

MIT License - See LICENSE file for details.
