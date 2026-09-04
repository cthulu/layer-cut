STEP 11 — CMake build system
  Goal: Single CMakeLists.txt that builds engine + CLI.
  Action: Root CMakeLists.txt + per-module CMakeLists.txt
  Tech:
    cmake_minimum_required(VERSION 3.20)
    project(cricut-slicer LANGUAGES CXX)

    add_library(engine STATIC engine/src/*.cpp)
    target_include_directories(engine PUBLIC engine/include)
    target_link_libraries(engine PUBLIC clipper2)

    add_executable(cricut-slicer cli/main.cpp)
    target_link_libraries(cricut-slicer PRIVATE engine cli11)

    - Dependencies: Clipper2 (git submodule),
      stb_image_write (header-only), CLI11 (header-only)
    - Build: cmake -B build -DCMAKE_BUILD_TYPE=Release
             cmake --build build
