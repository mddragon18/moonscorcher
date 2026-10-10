cmake_minimum_required(VERSION 3.28)
project(moonscorcher LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
# Prefer pkg-config NVML; fall back to plain libnvidia-ml.so.1 via dl.
pkg_check_modules(NVML IMPORTED_TARGET nvidia-ml)

# Vendor GL loader
add_library(glad STATIC third_party/glad/src/gl.c)
set_target_properties(glad PROPERTIES LINKER_LANGUAGE C)
target_include_directories(glad PUBLIC third_party/glad/include)

add_executable(moonscorcher
  src/main.cpp
  src/torus.cpp
  src/shaders.cpp
  src/gpumon.cpp
  src/bench.cpp
)

if(NVML_FOUND)
  target_link_libraries(moonscorcher PRIVATE PkgConfig::NVML)
endif()
target_link_libraries(moonscorcher PRIVATE glad glfw OpenGL::GL dl pthread)
target_compile_options(moonscorcher PRIVATE -O2 -Wall -Wextra)
