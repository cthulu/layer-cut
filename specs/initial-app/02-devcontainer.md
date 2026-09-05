STEP 2 — Create DevContainer for macOS
  Goal: Isolated build environment with CMake, C++17, build tools.
  Action: Create devcontainer/Dockerfile and devcontainer.json.
  Tech:
    - Validate whether the proposed macOS image can provide the required
      native macOS/Xcode environment; Docker normally provides Linux guests.
    - Pin the image version if it is usable; do not rely on `latest`.
    - Install: cmake, ninja, clang, pkg-config
    - Mount workspace as volume
    - Set CXX=clang++, CMAKE_CXX_STANDARD=17
  Fallback: Provide the primary supported macOS/Xcode build path plus a
    setup.sh that installs Homebrew deps: cmake ninja pkg-config
