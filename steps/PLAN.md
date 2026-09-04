# ============================================
# CRICUT-SLICER — Technical Implementation Plan
# ============================================
#
# Architecture Overview
#
#   cricut-slicer/
#   ├── engine/              # C++ core library (static .a)
#   │   ├── include/         # Public C ABI header
#   │   ├── src/             # Implementation
#   │   └── CMakeLists.txt
#   ├── cli/                 # Phase 1: CLI tool (links engine)
#   │   └── main.cpp
#   ├── macos-app/           # Phase 2: SwiftUI app (links engine)
#   │   └── CricutSlicerApp.swift
#   ├── devcontainer/        # DevContainer config
#   │   ├── Dockerfile
#   │   └── devcontainer.json
#   └── tests/               # Unit + integration tests
#
# ---------------------------------------------------------------
# PHASE 0: Environment Setup
# ---------------------------------------------------------------
#
# STEP 1 — Create project scaffold
#   Goal: Empty repo with folder structure.
#   Action: Create cricut-slicer/ with subfolders.
#   Tech: Standard folder creation. No code yet.
#
# STEP 2 — Create DevContainer for macOS
#   Goal: Isolated build environment with CMake, C++17, build tools.
#   Action: Create devcontainer/Dockerfile and devcontainer.json.
#   Tech:
#     - Base image: ghcr.io/cirruscontainers/macos/sequoia:latest
#     - Install: cmake, ninja, clang, pkg-config
#     - Mount workspace as volume
#     - Set CXX=clang++, CMAKE_CXX_STANDARD=17
#   Fallback: If DevContainer proves too complex, create
#     setup.sh that installs Homebrew deps: cmake ninja pkg-config
#
# ---------------------------------------------------------------
# PHASE 1: C++ Slicing Engine
# ---------------------------------------------------------------
#
# STEP 3 — STL binary parser
#   Goal: Parse binary STL files into a triangle mesh.
#   Action: engine/src/stl_loader.cpp + engine/include/stl_loader.h
#   Tech:
#     - Binary STL: 80-byte header + 4×uint32 (normal) +
#       3×float (vertex) × N triangles + 2-byte attr byte count
#     - Output: std::vector<Triangle>
#     - Validate triangle count ≤ 50M (memory guard)
#     - Unit test: Parse known STL, verify vertex count & bbox
#
# STEP 4 — Triangle mesh data structure
#   Goal: In-memory representation of the 3D model.
#   Action: engine/include/mesh.h
#   Tech:
#     struct Vec3 { float x, y, z; };
#     struct Triangle { Vec3 a, b, c; Vec3 normal; };
#     struct Mesh {
#         std::vector<Triangle> triangles;
#         Vec3 min, max;  // bounding box
#         float compute_volume() const;
#     };
#
# STEP 5 — Slicing engine (layer generation)
#   Goal: Convert 3D mesh into 2D cross-sections at layer heights.
#   Action: engine/src/slicer.cpp + engine/include/slicer.h
#   Tech:
#     - For each layer z = base + i × layerHeight:
#       find all triangles intersecting the plane
#     - For each triangle, check if vertices straddle the layer plane
#     - Compute 2D intersection points on the layer plane
#     - Output: std::vector<std::vector<Vec2>> per layer
#     - Sort points to form closed polygons (ear-clipping for concave)
#     - Unit test: Cube at 0.2mm → 100 layers, each a 20×20mm square
#
# STEP 6 — 2D boolean operations (Clipper2)
#   Goal: Merge overlapping polygons per layer into clean outlines.
#   Action: engine/src/polygon_ops.cpp + engine/include/polygon_ops.h
#   Tech:
#     - Link Clipper2 library (Zlib license, no AGPL concern)
#     - Operations: Union, Difference, Intersection of 2D polygons
#     - Per layer: union all polygons → single/multiple closed outlines
#     - Handle nested contours (holes) natively
#     - Unit test: Two overlapping circles → single merged polygon
#
# STEP 7 — SVG output generator
#   Goal: Export sliced layers as SVG files.
#   Action: engine/src/svg_writer.cpp + engine/include/svg_writer.h
#   Tech:
#     - Generate SVG <path> elements per polygon per layer
#     - Each layer gets a <g> group with layer index metadata
#     - Stroke width configurable (default 0.5mm → scaled to viewBox)
#     - Color per layer for visual distinction
#     - Optional: cut marks, registration marks
#     - Unit test: Generate SVG, verify valid XML & path data
#
# STEP 8 — PNG output generator (fallback)
#   Goal: Export sliced layers as 300 DPI PNG images.
#   Action: engine/src/png_writer.cpp + engine/include/png_writer.h
#   Tech:
#     - Use stb_image_write (header-only, public domain)
#     - Render SVG paths to raster at 300 DPI
#     - Black paths on white background (configurable)
#     - Filename: layer_N.png
#     - Unit test: Verify pixel dimensions match 300 DPI
#
# STEP 9 — C ABI bridge header
#   Goal: Expose engine functionality via a clean C interface for Swift/FFI.
#   Action: engine/include/cricut_slicer.h
#   Tech:
#     typedef void* slicer_mesh_t;
#     typedef void* slicer_result_t;
#
#     slicer_mesh_t slicer_load_stl(const char* path);
#     slicer_result_t slicer_slice(slicer_mesh_t mesh,
#                                  float layer_height_mm,
#                                  float width_mm, float height_mm);
#     int slicer_result_layer_count(slicer_result_t result);
#     const char* slicer_result_layer_svg(slicer_result_t result, int idx);
#     const char* slicer_result_layer_png_path(slicer_result_t result, int idx);
#     void slicer_free_mesh(slicer_mesh_t mesh);
#     void slicer_free_result(slicer_result_t result);
#
#     - All memory managed by engine (caller frees via *_free_*)
#     - No C++ types in the header — pure C
#     - Unit test: Call from a C program, verify correct output
#
# ---------------------------------------------------------------
# PHASE 2: CLI Tool
# ---------------------------------------------------------------
#
# STEP 10 — CLI tool implementation
#   Goal: Command-line slicer:
#     cricut-slicer model.stl -o output/ -l 0.2
#   Action: cli/main.cpp
#   Tech:
#     - Argument parsing: CLI11 (header-only, BSD-3)
#     - Flags: --input, --output-dir, --layer-height,
#               --width, --height, --format (svg|png)
#     - Default layer height: 0.2mm, default format: SVG
#     - Flow: load STL → slice → write SVG/PNG → print summary
#     - Exit codes: 0 = success, 1 = error, 2 = invalid input
#     - Unit test: Run CLI with test STL, verify output files
#
# STEP 11 — CMake build system
#   Goal: Single CMakeLists.txt that builds engine + CLI.
#   Action: Root CMakeLists.txt + per-module CMakeLists.txt
#   Tech:
#     cmake_minimum_required(VERSION 3.20)
#     project(cricut-slicer LANGUAGES CXX)
#
#     add_library(engine STATIC engine/src/*.cpp)
#     target_include_directories(engine PUBLIC engine/include)
#     target_link_libraries(engine PUBLIC clipper2)
#
#     add_executable(cricut-slicer cli/main.cpp)
#     target_link_libraries(cricut-slicer PRIVATE engine cli11)
#
#     - Dependencies: Clipper2 (git submodule),
#       stb_image_write (header-only), CLI11 (header-only)
#     - Build: cmake -B build -DCMAKE_BUILD_TYPE=Release
#              cmake --build build
#
# STEP 12 — Test STL model
#   Goal: Provide a known test model for verification.
#   Action: tests/assets/cube.stl (binary STL)
#   Tech:
#     - Simple 20mm × 20mm × 20mm cube
#     - Generate with Python script or Blender
#     - Expected: 100 layers at 0.2mm, each a 20×20mm square
#
# ---------------------------------------------------------------
# PHASE 3: macOS SwiftUI App
# ---------------------------------------------------------------
#
# STEP 13 — Swift package / Xcode project setup
#   Goal: Xcode project that links the C++ engine library.
#   Action: macos-app/ with CricutSlicerApp.swift, Info.plist
#   Tech:
#     - Xcode project (.xcodeproj) or Swift Package with native target
#     - Link libengine.a via Build Phases → Link Binary With Libraries
#     - Bridge header: #include "cricut_slicer.h"
#     - Swift interop: Use UnsafePointer<CChar> for C string args
#     - AI prompt suggestion:
#       "Create a SwiftUI macOS app that links a static C++ library
#        via a C ABI bridge header"
#
# STEP 14 — File import UI
#   Goal: User can drag-drop or browse for an STL file.
#   Action: macos-app/ImportView.swift
#   Tech:
#     - SwiftUI FileImporter or DropZone (drag-and-drop overlay)
#     - Validate file extension .stl
#     - Show file size and basic info (triangles, bounding box)
#     - Error handling: invalid STL, corrupted file, too large (>50MB)
#
# STEP 15 — Slicing parameters panel
#   Goal: Configurable layer height, model dimensions, output format.
#   Action: macos-app/ParametersPanel.swift
#   Tech:
#     - Sliders/steppers: layer height (0.1–0.3mm, step 0.01)
#     - Text fields: model width, height (auto-filled from STL)
#     - Segmented control: output format (SVG / PNG)
#     - "Slice" button triggers engine call
#     - Show estimated layer count and output file count
#
# STEP 16 — Engine integration (Swift ↔ C++)
#   Goal: Call the C++ slicer from Swift, receive SVG/PNG paths.
#   Action: macos-app/SlicingService.swift
#   Tech:
#     class SlicingService {
#         func slice(stlPath: String, layerHeight: Float,
#                    width: Float, height: Float)
#             throws -> [LayerOutput]
#         // Calls: slicer_load_stl → slicer_slice →
#         //        slicer_result_layer_svg → collect → free
#     }
#     - Error handling: throw Swift errors for C-level failures
#     - Progress reporting: callback or async sequence
#     - Memory management: ensure slicer_free_* called in defer blocks
#
# STEP 17 — Layer preview panel
#   Goal: Visual preview of sliced layers (scrollable thumbnails).
#   Action: macos-app/LayerPreviewView.swift
#   Tech:
#     - ScrollView with LazyVGrid of layer thumbnails
#     - SVG rendered via PDFDocument → NSImage (SwiftUI Image(nsImage:))
#     - PNG rendered directly via NSImage
#     - Click a layer → highlight in 3D viewport
#     - Layer count badge, zoom controls
#
# STEP 18 — 3D viewport (model preview)
#   Goal: Show the 3D model with a slice plane indicator.
#   Action: macos-app/Viewport3DView.swift
#   Tech:
#     - Use SceneKit (built into macOS) — no external deps
#     - Load STL via custom parser (reuse engine's STL loader)
#     - Draw a horizontal plane at the current layer's Z position
#     - Highlight active layer in preview panel when viewport interacted
#     - Orbit camera: pan, zoom, rotate via SceneKit SCNNode gestures
#     - AI prompt suggestion:
#       "Create a SceneKit 3D viewport in SwiftUI that loads a binary
#        STL and shows a movable slice plane"
#
# STEP 19 — Export / save workflow
#   Goal: User exports sliced files to a chosen directory.
#   Action: macos-app/ExportView.swift
#   Tech:
#     - FileImporter with directory permission
#     - Show output directory path, file count, total size
#     - "Export" button triggers batch write
#     - Progress bar during export
#     - Success/error toast notifications
#
# STEP 20 — Polish & packaging
#   Goal: App ready for distribution.
#   Action: Finalize UI, add settings, code sign, create .dmg.
#   Tech:
#     - Settings: default layer height, output format, DPI for PNG
#     - Dark mode support (SwiftUI automatic)
#     - Code sign: codesign --sign "Developer ID" CricutSlicer.app
#     - Create DMG: hdiutil create -format ADIF
#                    -srcfolder CricutSlicer.app cricut-slicer.dmg
#     - Optional: notarization via xcrun notarytool
#
# ---------------------------------------------------------------
# DEPENDENCY SUMMARY
# ---------------------------------------------------------------
#
#   Library              | Purpose                    | License  | Integration
#   ---------------------|----------------------------|----------|-------------
#   Clipper2             | 2D polygon boolean ops     | Zlib     | Git submodule
#   stb_image_write      | PNG generation             | Public   | Header-only
#   CLI11                | CLI argument parsing       | BSD-3    | Header-only
#   SceneKit             | 3D viewport (macOS)        | Apple    | System framework
#   PrusaSlicer/libslic3r| Advanced slicing (optional)| AGPL-3.0 | Deferred
#
#   NOTE on PrusaSlicer: Full integration is deferred. The initial
#   engine implements direct triangle-layer intersection (Steps 5-6),
#   which is sufficient for simple models (< 100K triangles).
#   PrusaSlicer's libslic3r can be added later as an optional backend
#   for advanced features (infill, supports, per-layer params).
#
# ---------------------------------------------------------------
# IMPLEMENTATION ORDER (Quick Reference)
# ---------------------------------------------------------------
#
#   01. Scaffold folders
#   02. DevContainer (Dockerfile + config)
#   03. STL parser
#   04. Mesh data structure
#   05. Slicer (layer generation)
#   06. Polygon ops (Clipper2)
#   07. SVG writer
#   08. PNG writer (stb_image_write)
#   09. C ABI bridge header
#   10. CLI tool (CLI11)
#   11. CMake build system
#   12. Test STL model
#   13. Xcode project setup
#   14. File import UI
#   15. Parameters panel
#   16. Engine ↔ Swift bridge
#   17. Layer preview panel
#   18. 3D viewport (SceneKit)
#   19. Export workflow
#   20. Polish & DMG packaging
#
# ---------------------------------------------------------------
# ESTIMATED COMPLEXITY PER STEP
# ---------------------------------------------------------------
#
#   Steps   | Complexity | Notes
#   --------|------------|----------------------------------------
#   01-02   | Trivial    | Setup only
#   03-04   | Low        | Well-defined binary format
#   05-06   | Medium     | Core algorithm — polygon sorting
#   07-08   | Low        | SVG is XML, PNG via stb is one call
#   09      | Low        | Thin wrapper, straightforward FFI
#   10-11   | Low        | CLI11 + CMake are well-documented
#   12      | Trivial    | One file
#   13-16   | Medium     | Swift ↔ C++ interop, careful pointers
#   17-18   | Medium     | SceneKit has a learning curve
#   19-20   | Low        | Standard macOS patterns
#
# ---------------------------------------------------------------
# This plan is structured so each step produces a verifiable
# artifact (code that compiles + tests that pass). An AI agent
# can execute these sequentially with clear success criteria.
# ---------------------------------------------------------------
