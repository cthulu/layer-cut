cmake_minimum_required(VERSION 3.20)

project(layer-cut VERSION 0.1.0 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

option(LAYER_CUT_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(LAYER_CUT_FETCH_DEPENDENCIES "Fetch pinned third-party dependencies" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(layer_cut_warnings INTERFACE)
if(MSVC)
  target_compile_options(layer_cut_warnings INTERFACE /W4 /permissive-)
else()
  target_compile_options(layer_cut_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

if(LAYER_CUT_ENABLE_SANITIZERS AND NOT MSVC)
  add_library(layer_cut_sanitizers INTERFACE)
  target_compile_options(layer_cut_sanitizers INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
  target_link_options(layer_cut_sanitizers INTERFACE -fsanitize=address,undefined)
endif()

if(LAYER_CUT_FETCH_DEPENDENCIES)
  include(FetchContent)

  FetchContent_Declare(
    clipper2
    GIT_REPOSITORY https://github.com/AngusJohnson/Clipper2.git
    GIT_TAG Clipper2_1.5.4
    GIT_SHALLOW TRUE
  )
  FetchContent_Declare(
    cli11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.5.0
    GIT_SHALLOW TRUE
  )
  FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG 2c980bb59875b0d32144a71867fbdebb2f77cd20
    GIT_SHALLOW FALSE
  )
  # Clipper2 and CLI11 provide CMake projects. stb is header-only and does
  # not, so populate it without adding it as a subdirectory.
  FetchContent_MakeAvailable(clipper2 cli11)
  FetchContent_GetProperties(stb)
  if(NOT stb_POPULATED)
    FetchContent_Populate(stb)
  endif()

  add_library(stb_image_write INTERFACE)
  target_include_directories(stb_image_write SYSTEM INTERFACE
    ${stb_SOURCE_DIR}
  )
endif()

add_subdirectory(engine)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()

message(STATUS "layer-cut ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Sanitizers: ${LAYER_CUT_ENABLE_SANITIZERS}")
message(STATUS "  Fetch dependencies: ${LAYER_CUT_FETCH_DEPENDENCIES}")
