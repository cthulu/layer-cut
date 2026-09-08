# Layer Cut

Layer Cut converts 3D STL models into 2D layers for Cricut and other cutting
machines. It produces dimensionally accurate SVG or PNG files, and can also
arrange layers into Cricut-compatible pages. A C++17 engine powers both the
command-line tool and the macOS SwiftUI application.

## Features

- Slice STL meshes into horizontal XY layers at a configurable layer height.
- Validate and normalize mesh data before slicing.
- Reconstruct and clean contours with Clipper2 polygon operations.
- Export numbered SVG layers or PNG layers with physical DPI metadata.
- Detect or apply manufacturing limits for narrow features, bridges, holes,
  and small islands.
- Generate Cricut normal or large pages with fixed or tight packing, cut and
  guide SVGs, and optional layer numbers.
- Generate a stacked STL preview of the sliced layers.
- Use the macOS app for STL import, profile-based settings, a RealityKit 3D
  viewport, layer previews, diagnostics, and export.

## Quick Start

### CLI

Requirements: CMake 3.20 or newer, a C++17 compiler, and Ninja (recommended).
Build the CLI with the checked-in script, which configures the pinned
dependencies automatically:

```bash
./tools/build_cli.sh
```

The script places the executable at `build/cli/cli/layer-cut` by default. Set
`LAYER_CUT_BUILD_DIR` to use a different build directory.

Slice an STL into SVG layers:

```bash
./build/cli/cli/layer-cut model.stl -o output -l 0.2 --format svg
```

Use `--help` for all options:

```bash
./build/cli/cli/layer-cut --help
```

Output files are named `layer_000.svg`, `layer_001.svg`, and so on. SVG
dimensions are in millimetres. PNG output is available with
`--format png`; use `--dpi` to select its physical resolution.

For Cricut page output:

```bash
./build/cli/cli/layer-cut model.stl -o output -l 0.2 \
  --format cricut-normal --packing tight --cricut-combined \
  --layer-numbers
```

Manufacturing cleanup defaults to `--cleanup warn`, which preserves geometry
and reports features below the configured limits. Use `--cleanup apply` to
remove or close features that do not meet the selected limits, or
`--cleanup preserve` to suppress cleanup warnings.

## macOS App

The native application requires macOS 14.0 or later, full Xcode, CMake 3.20+,
and Ninja. Verify the toolchain and build the generated Xcode project with:

```bash
./tools/check_macos_tooling.sh
./tools/build_macos_app.sh
```

The generated project is written to `build/macos-xcode/layer-cut.xcodeproj`.
Open it in Xcode with:

```bash
open build/macos-xcode/layer-cut.xcodeproj
```

The first release target is arm64. Release packaging, signing, DMG creation,
and optional notarization are documented in
[`docs/macos-release.md`](docs/macos-release.md).

## Development

Build the engine and run its tests without fetching optional dependencies:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

Enable dependency-backed CLI and polygon-operation tests with
`-DLAYER_CUT_FETCH_DEPENDENCIES=ON`. Enable AddressSanitizer and
UndefinedBehaviorSanitizer with:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DLAYER_CUT_ENABLE_SANITIZERS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

The project is organized into the C++ engine, CLI, macOS app, and tests. The
Swift app communicates with the engine through the plain C ABI in
`engine/include/layer_cut.h`.

## Documentation

- [`BUILDING.md`](BUILDING.md) contains platform-specific build details and
  CLI behavior.
- [`docs/macos-release.md`](docs/macos-release.md) documents macOS release
  packaging and the location of the generated DMG.
- [`macos-app/README.md`](macos-app/README.md) documents the app architecture
  and development notes.
- [`THIRD_PARTY_LICENSES.md`](THIRD_PARTY_LICENSES.md) lists dependency
  license information.
