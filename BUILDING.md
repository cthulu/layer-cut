# Building layer-cut

## Native macOS

Run the reproducible setup script first. It installs Xcode Command Line Tools
prerequisites on macOS and the required packages on Debian-based Linux:

```bash
./devcontainer/setup.sh
```

The script installs CMake 3.20 or newer, Ninja, pkg-config, and a Clang/C++17
toolchain.

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Enable sanitizers for development builds:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DLAYER_CUT_ENABLE_SANITIZERS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Cross-platform

The same commands work on Linux and Windows with a C++17 compiler and CMake.
Omit `-G Ninja` if Ninja is not installed and use the platform's default
generator.

Third-party dependencies are pinned in the root `CMakeLists.txt`. They are
fetched only when explicitly enabled:

```bash
cmake -S . -B build -G Ninja -DLAYER_CUT_FETCH_DEPENDENCIES=ON
```
