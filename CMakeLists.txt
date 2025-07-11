cmake_minimum_required(VERSION 3.16)
project(NBodySimulation VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set build type to Release by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
if(MSVC)
    add_compile_options(/W4 /permissive- /arch:AVX2)
    add_compile_options("$<$<CONFIG:RELEASE>:/O2>")
else()
    add_compile_options(-Wall -Wextra -Wpedantic -march=native)
    add_compile_options("$<$<CONFIG:DEBUG>:-g>")
    add_compile_options("$<$<CONFIG:RELEASE>:-O3>")
endif()

# Find packages
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glew REQUIRED)

# Find GLM
find_package(glm REQUIRED)

# Find ImGui (if using vcpkg)
find_package(imgui REQUIRED)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
)

file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    OpenGL::GL
    glfw
    GLEW::GLEW
    glm::glm
    imgui::imgui
)

# Copy shaders and assets to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Platform-specific settings
if(WIN32)
    # Console application (remove WIN32_EXECUTABLE)
endif()

# Enable parallel compilation on MSVC
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/MP")
endif()
