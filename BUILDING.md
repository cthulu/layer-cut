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

Load a binary STL, slice it, and export numbered SVG layers:

```bash
./build/cli/layer-cut testfiles/fox.stl -o output -l 0.2 --format svg
```

The same build can be performed with the checked-in helper script. It uses
`build/cli` by default; set `LAYER_CUT_BUILD_DIR` to override that location:

```bash
./tools/build_cli.sh
./build/cli/cli/layer-cut --help
```

The CLI validates and normalizes the binary STL, slices it at midpoint layer
positions, cleans contours with Clipper2, and writes files named
`layer_000.svg`, `layer_001.svg`, and so on. SVG dimensions are in millimetres
and use the normalized model XY bounds unless an explicit canvas is supplied.
PNG output is also available with `--format png`; dimensions are derived from
the physical canvas bounds and DPI, and PNG resolution metadata is embedded.

Manufacturing cleanup defaults to `--cleanup warn`, which preserves geometry
and reports undersized features. Use `--cleanup preserve` to suppress those
warnings, or `--cleanup apply` to explicitly remove narrow features, close
narrow holes, and filter small islands. Configure limits with
`--min-feature-width`, `--min-hole-width`, `--min-bridge-width`, and
`--min-area` (square millimetres).

The engine tests now cover mesh normalization, triangle-plane intersections,
contour reconstruction, layer scheduling, and Clipper2 polygon operations when
dependency fetching is enabled.

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

## Build the macOS App

On macOS, verify full Xcode and the required command-line tools, then run the
checked-in build script:

```bash
./tools/check_macos_tooling.sh
./tools/build_macos_app.sh
```

The script generates `build/macos-xcode/layer-cut.xcodeproj`, selects `arm64`,
and builds the `LayerCutApp` scheme with `xcodebuild`. Xcode derived data is
stored in a unique directory below `TMPDIR` (or `/tmp` when `TMPDIR` is unset)
and removed automatically; no machine-specific temporary path is required.
