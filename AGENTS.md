# Layer Cut (Cricut Slicer) 

## What This Is

A cross-platform STL 3D model slicer that converts 3D prints into 2D layers for cutting machines (Cricut, etc.). Outputs layers as SVG (primary) or PNG (fallback).

## Architecture

```
cricut-slicer/
├── engine/              # C++ core library (static .a)
│   ├── include/         # Public C ABI header
│   ├── src/             # Implementation
│   └── CMakeLists.txt
├── cli/                 # Phase 1: CLI tool (links engine)
│   └── main.cpp
├── macos-app/           # Phase 2: SwiftUI app (links engine)
├── devcontainer/        # DevContainer config
└── tests/               # Unit + integration tests
```

## Technical Decisions

- **C++17** core engine, compiled as static library (`.a`)
- **C ABI bridge** (`cricut_slicer.h`) for Swift/FFI interop — no C++ types in the header
- **Clipper2** (Zlib) for 2D polygon boolean operations, linked as git submodule
- **stb_image_write** (public domain, header-only) for PNG generation
- **CLI11** (BSD-3, header-only) for CLI argument parsing
- **SceneKit** (Apple system framework) for 3D viewport in macOS app
- **DevContainer** with `ghcr.io/cirruscontainers/macos/sequoia:latest` base image
- Build system: **CMake 3.20+** with Ninja

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Implementation Order

Steps are numbered 01–20 and stored in `steps/`. Execute sequentially — each step produces a verifiable artifact (compiling code + passing tests).

## Dependencies

| Library              | Purpose                    | License  | Integration  |
|----------------------|----------------------------|----------|--------------|
| Clipper2             | 2D polygon boolean ops     | Zlib     | Git submodule  |
| stb_image_write      | PNG generation             | Public   | Header-only  |
| CLI11                | CLI argument parsing       | BSD-3    | Header-only  |
| SceneKit             | 3D viewport (macOS)        | Apple    | System framework |

PrusaSlicer/libslic3r integration is deferred (AGPL-3.0).

## Updating This File

Update this AGENTS.md after completing groups of steps to reflect current state.
