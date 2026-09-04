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

## Run the CLI shell

The CLI target requires the pinned CLI11 dependency, so configure with
dependency fetching enabled:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DLAYER_CUT_FETCH_DEPENDENCIES=ON
cmake --build build --target layer-cut
```

Show the available options:

```bash
./build/cli/layer-cut --help
```

Load a binary STL and print its metadata and requested configuration:

```bash
./build/cli/layer-cut model.stl -o output -l 0.2 --format svg --dpi 300
```

The current CLI is a shell: it validates arguments, loads binary STL files,
and prints metadata. Slicing and SVG/PNG file generation will be added with
the remaining engine steps.

## Run tests

After configuring and building, run all registered tests with:

```bash
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
