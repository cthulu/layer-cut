STEP 11 — CMake build system
  Goal: Complete CMake configuration that builds engine, CLI, and tests.
  Action: Root CMakeLists.txt + per-module CMakeLists.txt
  Tech:
    cmake_minimum_required(VERSION 3.20)
    project(layer-cut LANGUAGES CXX)

    add_library(engine STATIC <explicit engine source list>)
    target_include_directories(engine PUBLIC engine/include)
    target_link_libraries(engine PUBLIC clipper2)

    add_executable(layer-cut cli/main.cpp)
    target_link_libraries(layer-cut PRIVATE engine cli11)

    - List source files explicitly; CMake does not expand `*.cpp` globs.
    - Pin and document dependencies: Clipper2 (git submodule),
      stb_image_write (header-only), CLI11 (header-only)
    - Finalize CTest targets, dependency initialization instructions, warnings,
      development sanitizers, install rules, and arm64/x86_64 build checks.
    - This finalizes the bootstrap from Step 2A; it must not be the first
      point at which the plan introduces a buildable test target.
    - Build: cmake -B build -DCMAKE_BUILD_TYPE=Release
             cmake --build build
