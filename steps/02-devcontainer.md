STEP 2 — Create DevContainer for macOS
  Goal: Isolated build environment with CMake, C++17, build tools.
  Action: Create devcontainer/Dockerfile and devcontainer.json.
  Tech:
    - Base image: ghcr.io/cirruscontainers/macos/sequoia:latest
    - Install: cmake, ninja, clang, pkg-config
    - Mount workspace as volume
    - Set CXX=clang++, CMAKE_CXX_STANDARD=17
  Fallback: If DevContainer proves too complex, create
    setup.sh that installs Homebrew deps: cmake ninja pkg-config
