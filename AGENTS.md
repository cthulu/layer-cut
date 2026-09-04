# Layer Cut (Cricut Slicer) 

## What This Is

A cross-platform STL 3D model slicer that converts 3D prints into 2D layers for cutting machines (Cricut, etc.). Outputs layers as SVG (primary) or PNG (fallback).

## Architecture

```
layer-cut/
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
- **C ABI bridge** (`layer_cut.h`) for Swift/FFI interop — no C++ types in the header
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
ctest --test-dir build --output-on-failure
```

Dependency fetching is opt-in with `-DLAYER_CUT_FETCH_DEPENDENCIES=ON`;
see `BUILDING.md` for native macOS and cross-platform instructions.

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

## Commit Conventions

Use brief commit messages with a maximum of 2–3 bullet points.
No long descriptions, no preamble, no postamble.

Example:
```
feat: add STL binary parser

- Parse binary STL files with validation and limits
- Add unit tests for valid, truncated, ASCII, and malformed input
```

## Current State

- Steps 01-12, including substep 06A, are implemented and covered by CTest.
- Step 08 is implemented: PNG rasterization uses even-odd coverage,
  stb_image_write encoding, physical DPI metadata, and bounded allocations.
- Step 09 is implemented: the pure C ABI exposes opaque mesh/config/result
  handles, thread-local diagnostics, SVG/PNG access, and cleanup APIs.
- Step 12 includes deterministic fixtures for cubes, negative bounds,
  disconnected components, concavity, holes, and non-divisible heights.
- Steps 13-20 (macOS SwiftUI application and packaging) remain.

## Updating This File

Update this AGENTS.md after completing groups of steps to reflect current state.
