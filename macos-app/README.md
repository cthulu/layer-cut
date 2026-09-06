# LayerCutApp

macOS SwiftUI application for Layer Cut — a 3D model slicer for 2D cutting machines.

## Quick Start

### Prerequisites

- macOS 14.0 (Sonoma) or later
- Full Xcode (Command Line Tools alone are insufficient)
- CMake 3.20+
- Ninja

### Verify Tooling

```bash
cd ..
./tools/check_macos_tooling.sh
```

### Build the macOS App

```bash
cd ..
./tools/build_macos_app.sh
```

This generates an Xcode project with CMake and builds the C++17 engine as an
Xcode target. The app links that target directly; no manually copied static
library is required. Xcode derived data is created in a unique temporary
directory and removed when the script exits.

The first release targets `arm64` only. The architecture is passed explicitly
to CMake and is verified by the Xcode build.

### Open the Generated Project

```bash
open ../build/macos-xcode/layer-cut.xcodeproj
```

## Architecture

- See [`ARCHITECTURE.md`](ARCHITECTURE.md) for the frontend and engine
  boundary contract.
- **SwiftUI** — macOS application UI
- **RealityKit** — Interactive 3D viewport (deferred)
- **C++17 Engine** — STL parsing, slicing, SVG/PNG/Cricut export
- **C ABI Bridge** — Pure C interface for Swift interop (`layer_cut.h`)
- **YAML Profiles** — Persisted slicing parameters (deferred)

## File Structure

```
macos-app/
├── Sources/
│   └── LayerCutApp/
│       ├── LayerCutApp.swift
│       ├── EngineBridge.swift
│       └── ViewportView.swift
└── CMakeLists.txt             # Xcode project target definition
```

## Development Notes

- The application target is generated from the root CMake project.
- The C ABI (`layer_cut.h`) is the contract between the C++ engine and Swift.
  All Swift engine bindings must use opaque handles with explicit cleanup.
- The RealityKit viewport orbits with a secondary-button drag, while the
  camera interaction remains separate from engine transforms.
- YAML profile persistence is deferred; the settings view is a placeholder.
