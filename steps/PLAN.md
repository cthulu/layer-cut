# ============================================
# LAYER-CUT — Technical Implementation Plan
# ============================================
#
# Architecture Overview
#
#   layer-cut/
#   ├── engine/              # C++ core library (static .a)
#   │   ├── include/         # Public C ABI header
#   │   ├── src/             # Implementation
#   │   └── CMakeLists.txt
#   ├── cli/                 # Phase 1: CLI tool (links engine)
#   │   └── main.cpp
#   ├── macos-app/           # Phase 2: SwiftUI app (links engine)
#   │   └── LayerCutApp.swift
#   ├── devcontainer/        # DevContainer config
#   │   ├── Dockerfile
#   │   └── devcontainer.json
#   └── tests/               # Unit + integration tests
#
# ---------------------------------------------------------------
# VERSION 1 SCOPE AND GEOMETRY CONTRACT
# ---------------------------------------------------------------
#
#   - STL coordinates are interpreted as millimetres. STL has no reliable
#     unit metadata; show this assumption in CLI help and the UI.
#   - The input Z axis is the slicing axis. Use horizontal XY planes.
#   - Translate the mesh so minZ becomes 0 before slicing. Do not rotate or
#     auto-orient models in version 1; rotation is a later feature.
#   - Preserve X/Y coordinates. Output bounds derive from the normalized mesh
#     unless an explicit output canvas is requested.
#   - Sample layers at z = (i + 0.5) * layerHeight over [0, modelHeight).
#     Therefore a 20 mm cube at 0.2 mm produces 100 layers and no duplicate
#     zero-thickness top layer.
#   - Triangle-plane hits produce segments. Deduplicate, endpoint-snap, and
#     join segments into closed contours before polygon operations.
#   - Reject empty and fundamentally unreadable meshes. Attempt best-effort
#     slicing for open, non-manifold, self-intersecting, and degenerate input,
#     while returning visible warnings and per-layer diagnostics. Never claim
#     that such output is a valid solid.
#   - SVG uses filled paths, a millimetre viewBox, and an explicit even-odd
#     fill rule. No decorative stroke is emitted by default.
#   - PNG requires a rasterizer; stb_image_write only encodes pixel buffers.
#   - The engine owns geometry and serialization through one documented
#     configuration/result API used by both CLI and Swift clients.
#   - Export one automatically numbered SVG or PNG per layer into the
#     user-provided output directory. SVG preserves millimetre dimensions;
#     PNG dimensions derive from the same physical bounds and explicit DPI.
#
# ---------------------------------------------------------------
# PHASE 0: Environment Setup
# ---------------------------------------------------------------
#
# STEP 1 — Create project scaffold
#   Goal: Empty repo with folder structure.
#   Action: Create layer-cut/ with subfolders.
#   Tech: Standard folder creation. No code yet.
#
# STEP 2 — Define development environment
#   Goal: Documented native macOS primary environment with CMake, C++17, and build tools.
#   Action: Create devcontainer/Dockerfile and devcontainer.json.
#   Tech:
#     - First validate whether the proposed image provides a native macOS/
#       Xcode environment; Docker normally provides Linux guests.
#     - Pin the image version if usable; do not rely on `latest`.
#     - Install: cmake, ninja, clang, pkg-config
#     - Mount workspace as volume
#     - Set CXX=clang++, CMAKE_CXX_STANDARD=17
#   Fallback: Provide the primary supported macOS/Xcode build path plus a
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
#     - Binary STL: 80-byte header + uint32 triangle count +
#       3×float (vertex) × N triangles + 2-byte attribute byte count
#     - Output: std::vector<Triangle>
#     - Validate file length before allocation, detect truncation and integer
#       overflow, require finite coordinates, and enforce configurable byte,
#       triangle, and memory limits. Version 1 accepts binary STL only.
#     - Unit tests cover valid, truncated, oversized, non-finite, and malformed
#       files, including an ASCII file beginning with `solid`.
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
#         double compute_volume() const;
#     };
#   - Use double for bounds and geometric calculations where practical.
#   - Define volume behavior for open, inverted, degenerate, and
#     self-intersecting meshes; do not use volume alone as a validity check.
#
# STEP 4A — Mesh validation and normalization
#   Goal: Establish a safe, normalized solid before slicing.
#   Action: engine/src/mesh_validation.cpp + tests.
#   Tech:
#     - Reject empty/non-finite input. Detect degenerate, open, non-manifold,
#       and self-intersecting input, but continue best-effort where possible.
#     - Emit structured warnings identifying unsafe topology and affected
#       operations; never claim that best-effort output is a valid solid.
#     - Compute bounds and translate minZ to zero without changing X/Y.
#     - Return structured diagnostics and test negative coordinates, duplicate
#       vertices, inverted winding, disconnected solids, and invalid topology.
#
# STEP 5A — Triangle-plane intersection primitives
#   Goal: Correctly convert individual triangles into plane segments.
#   Action: engine/src/plane_intersection.cpp + headers/tests.
#   Tech:
#     - Define epsilon rules for below/on/above-plane vertices.
#     - Handle vertex touches, coplanar edges, and coplanar triangles without
#       zero-length or duplicate segments.
#     - Return 2D XY segments with enough identity to deduplicate shared edges.
#     - Test every sign/topology and numerical boundary case.
#
# STEP 5B — Segment deduplication and contour reconstruction
#   Goal: Turn plane segments into oriented closed contours.
#   Action: engine/src/contour_builder.cpp + headers/tests.
#   Tech:
#     - Snap endpoints within a documented tolerance using spatial hashing.
#     - Deduplicate shared edges and remove zero-length segments.
#     - Build loops from an adjacency graph, never by globally sorting points.
#     - Detect dangling edges/branching and return diagnostics.
#     - Classify outer contours and holes by signed area and normalize winding.
#     - Test concave shapes, holes, disconnected solids, and touching parts.
#
# STEP 5C — Layer scheduling and slicing orchestration
#   Goal: Generate deterministic layers from normalized mesh contours.
#   Action: engine/src/slicer.cpp + engine/include/slicer.h.
#   Tech:
#     - Validate positive finite layer height and sample z = (i + 0.5) * h.
#     - Use [0, modelHeight), never emit a duplicate top boundary layer.
#     - Return contours plus layer Z, bounds, and diagnostics.
#     - Test a normalized 20 mm cube at 0.2 mm: 100 layers, each 20×20 mm.
#
# STEP 6 — 2D boolean operations (Clipper2)
#   Goal: Merge overlapping polygons per layer into clean outlines.
#   Action: engine/src/polygon_ops.cpp + engine/include/polygon_ops.h
#   Tech:
#     - Link Clipper2 library (Zlib license, no AGPL concern)
#     - Use a documented fixed integer scale, rounding mode, coordinate bounds,
#       tolerance, fill rule, and winding policy. Reject integer overflow.
#     - Operations: Union, Difference, Intersection of 2D polygons
#     - Per layer: union all polygons → single/multiple closed outlines
#     - Handle nested contours (holes) natively
#     - Unit test: Two overlapping circles → single merged polygon
#
# STEP 7 — SVG output generator
#   Goal: Export one physically sized SVG file per numbered layer.
#   Action: engine/src/svg_writer.cpp + engine/include/svg_writer.h
#   Tech:
#     - Generate SVG <path> elements per polygon per layer
#     - Each layer gets a <g> group with layer index metadata
#     - Use filled paths with `fill-rule="evenodd"`; omit stroke by default
#       because stroke changes the physical cut boundary.
#     - Use a millimetre viewBox matching explicit output bounds and document
#       origin and Y-axis orientation. Colors are optional presentation only.
#     - Optional: cut marks, registration marks
#     - Unit test: Verify XML, millimetre viewBox, path closure, fill rule,
#       holes, bounds, and coordinates against known geometry.
#
# STEP 8 — PNG output generator (fallback)
#   Goal: Export one physically sized PNG file per numbered layer.
#   Action: engine/src/png_writer.cpp + engine/include/png_writer.h
#   Tech:
#     - Rasterize contours directly or use a declared, tested rasterizer;
#       stb_image_write only encodes the resulting pixel buffer.
#     - Convert dimensions as ceil(canvas_mm / 25.4 * dpi), with documented
#       bounds, antialiasing, fill rule, and background semantics.
#     - Embed the requested DPI in PNG resolution metadata so physical size is
#       preserved by consumers that honor PNG metadata.
#     - Use stb_image_write (header-only, public domain) for encoding.
#     - Black paths on white background (configurable)
#     - Filename: layer_N.png with deterministic zero-padding policy.
#     - Unit test: Verify pixel dimensions match 300 DPI
#
# STEP 9 — C ABI bridge header
#   Goal: Expose engine functionality via a clean C interface for Swift/FFI.
#   Action: engine/include/layer_cut.h
#   Tech:
#     typedef void* slicer_mesh_t;
#     typedef void* slicer_result_t;
#     typedef void* slicer_config_t;
#
#     slicer_mesh_t slicer_load_stl(const char* path);
#     slicer_config_t slicer_config_create(void);
#     slicer_result_t slicer_slice(slicer_mesh_t mesh,
#                                  slicer_config_t config);
#     int slicer_result_layer_count(slicer_result_t result);
#     const char* slicer_result_layer_svg(slicer_result_t result, int idx);
#     const uint8_t* slicer_result_layer_png(slicer_result_t result, int idx,
#                                            size_t* size);
#     const char* slicer_last_error(void);
#     void slicer_free_config(slicer_config_t config);
#     void slicer_free_mesh(slicer_mesh_t mesh);
#     void slicer_free_result(slicer_result_t result);
#
#     - Config carries layer height, format, DPI, output bounds, and
#       normalization policy. Define returned-byte lifetimes and copy rules.
#     - Return status codes plus retrievable diagnostics; null alone is not an
#       adequate error contract. All memory is managed by engine.
#     - No C++ types in the header — pure C
#     - Unit test: Call from a C program, verify correct output
#
# ---------------------------------------------------------------
# PHASE 2: CLI Tool
# ---------------------------------------------------------------
#
# STEP 10 — CLI tool implementation
#   Goal: Command-line slicer:
#     layer-cut model.stl -o output/ -l 0.2
#   Action: cli/main.cpp
#   Tech:
#     - Argument parsing: CLI11 (header-only, BSD-3)
#     - Use one consistent input syntax. Define flags for --output-dir,
#       --layer-height, --dpi, --format (svg|png), and optional canvas bounds.
#     - State the millimetre, Z/XY, and min-Z normalization conventions in help.
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
#     project(layer-cut LANGUAGES CXX)
#
#     add_library(engine STATIC <explicit engine source list>)
#     target_include_directories(engine PUBLIC engine/include)
#     target_link_libraries(engine PUBLIC clipper2)
#
#     add_executable(layer-cut cli/main.cpp)
#     target_link_libraries(layer-cut PRIVATE engine cli11)
#
#     - List source files explicitly; CMake does not expand `*.cpp` globs.
#     - Pin and document dependencies: Clipper2 (git submodule),
#       stb_image_write (header-only), CLI11 (header-only)
#     - Add CTest targets, dependency initialization instructions, warnings,
#       development sanitizers, and arm64/x86_64 build checks.
#     - Build: cmake -B build -DCMAKE_BUILD_TYPE=Release
#              cmake --build build
#
# STEP 12 — Test STL model
#   Goal: Provide a known test model for verification.
#   Action: tests/assets/cube.stl (binary STL)
#   Tech:
#     - Simple 20mm × 20mm × 20mm cube
#     - Generate with Python script or Blender
#     - After min-Z normalization, expected: 100 layers at 0.2mm, each a
#       20×20mm square. Assert coordinates and areas with tolerance.
#     - Add fixtures for holes, concavity, tilted faces, disconnected solids,
#       negative coordinates, malformed input, and non-divisible heights.
#
# ---------------------------------------------------------------
# PHASE 3: macOS SwiftUI App
# ---------------------------------------------------------------
#
# STEP 13 — Swift package / Xcode project setup
#   Goal: Xcode project that links the C++ engine library.
#   Action: macos-app/ with LayerCutApp.swift, Info.plist
#   Tech:
#     - Choose one reproducible integration model: Xcode project or Swift
#       Package with a native/binary target.
#     - Link libengine.a plus libc++, set deployment target and architecture,
#       and verify arm64 (and x86_64 if supported).
#     - Bridge header: #include "layer_cut.h"
#     - Swift interop: Use UnsafePointer<CChar> for C string args and import
#       the C header with fixed-width integer includes and ownership rules.
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
#     - Error handling: invalid STL, corrupted file, unsupported ASCII STL,
#       and configurable byte/triangle/memory limits.
#
# STEP 15 — Slicing parameters panel
#   Goal: Configurable layer height, model dimensions, output format.
#   Action: macos-app/ParametersPanel.swift
#   Tech:
#     - Sliders/steppers: layer height (0.1–0.3mm, step 0.01)
#     - Show normalized model X/Y/Z bounds and distinguish them from optional
#       output canvas width/height; never silently scale.
#     - Segmented control: output format (SVG / PNG)
#     - "Slice" button triggers engine call
#     - Show estimated layer count and output file count
#
# STEP 16 — Engine integration (Swift ↔ C++)
#   Goal: Call the C++ slicer from Swift and receive owned SVG/PNG bytes.
#   Action: macos-app/SlicingService.swift
#   Tech:
#     class SlicingService {
#         func slice(stlPath: String, layerHeight: Float,
#                    width: Float, height: Float)
#             throws -> [LayerOutput]
#         // Calls: slicer_load_stl → slicer_slice →
#         //        slicer_result_layer_svg → collect → free
#     }
#     - Throw Swift errors from C status codes and retrievable diagnostics.
#     - Progress/cancellation must be in the C ABI before exposing them, or be
#       explicitly deferred. Never block the main actor during slicing.
#     - Memory management: ensure slicer_free_* called in defer blocks
#
# STEP 17 — Layer preview panel
#   Goal: Visual preview of sliced layers (scrollable thumbnails).
#   Action: macos-app/LayerPreviewView.swift
#   Tech:
#     - ScrollView with LazyVGrid of layer thumbnails
#     - Request engine-generated PNG preview bytes at a UI-appropriate DPI and
#       render them directly via NSImage; do not add a separate SVG renderer.
#     - Click a layer → highlight in 3D viewport
#     - Layer count badge, zoom controls
#
# STEP 18 — 3D viewport (model preview)
#   Goal: Show the 3D model with a slice plane indicator.
#   Action: macos-app/Viewport3DView.swift
#   Tech:
#     - Use SceneKit (built into macOS) — no external deps
#     - Load the validated/normalized mesh through the engine boundary; avoid
#       a second Swift parser with different units or topology behavior.
#     - Draw a horizontal plane at the current layer's Z position
#     - Highlight active layer in preview panel when viewport interacted
#     - Orbit camera: pan, zoom, rotate via SceneKit SCNNode gestures
#     - AI prompt suggestion:
#       "Create a SceneKit 3D viewport in SwiftUI that loads a binary
#        STL and shows a movable slice plane"
#
# STEP 19 — Export / save workflow
#   Goal: User exports one automatically numbered, physically sized file per
#     layer into the chosen output directory.
#   Action: macos-app/ExportView.swift
#   Tech:
#     - FileImporter with directory permission
#     - Show output directory path, file count, total size, and deterministic
#       naming (for example `layer_001.svg`).
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
#     - Code sign: codesign --sign "Developer ID" LayerCut.app
#     - Create DMG: hdiutil create -format ADIF
#                    -srcfolder LayerCut.app layer-cut.dmg
#     - Include hardened runtime, entitlements, architecture/universal-binary
#       decision, reproducible versioning, and notarization prerequisites.
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
#   02A. Build and test bootstrap
#   03. STL parser
#   04. Mesh data structure
#   04A. Mesh validation and normalization
#   05A. Triangle-plane intersection
#   05B. Segment deduplication and contour reconstruction
#   05C. Layer scheduling and slicing orchestration
#   06. Polygon ops (Clipper2)
#   07. SVG writer
#   08. PNG rasterization and writer
#   09. C ABI bridge header
#   10. CLI tool (CLI11)
#   11. Finalize CMake, dependencies, and CTest
#   12. Test STL model
#   13. Xcode project setup
#   14. File import UI
#   15. Parameters panel
#   16. Engine ↔ Swift bridge
#   17. Layer preview renderer and panel
#   18. 3D viewport (SceneKit)
#   19. Export workflow
#   20. Polish, signing, notarization, and DMG packaging
#
# ---------------------------------------------------------------
# ESTIMATED COMPLEXITY PER STEP
# ---------------------------------------------------------------
#
#   Steps   | Complexity | Notes
#   --------|------------|----------------------------------------
#   01-02   | Trivial    | Setup only
#   03-04   | Low        | Parsing, validation, and normalization
#   05A-05C | High       | Robust geometric reconstruction
#   06      | Medium     | Numeric conversion and polygon cleanup
#   07-08   | Medium     | Physical cut output and rasterization
#   09      | Low        | Thin wrapper, straightforward FFI
#   10-11   | Low        | CLI11 + CMake are well-documented
#   12      | Trivial    | One file
#   13-16   | Medium     | Swift ↔ C++ interop, careful pointers
#   17-18   | Medium     | SceneKit has a learning curve
#   19-20   | Low        | Standard macOS patterns
#
# ---------------------------------------------------------------
# Each step now has a bounded artifact and explicit acceptance tests. Build
# infrastructure and test targets must exist before implementation steps claim
# compilation; no step may rely on an unstated geometry or memory policy.
# ---------------------------------------------------------------
