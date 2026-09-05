# STEP 13 - SwiftUI Xcode Project

Priority: P0

## Goal

Create a reproducible macOS SwiftUI application target that links the C++ engine.

## Requirements

- Use an Xcode project with a local Swift package only for third-party Swift
  dependencies such as YAML parsing.
- Create `macos-app/LayerCutApp.swift` and app target sources.
- Link the C++ static library and `libc++` through a documented build phase.
- Keep the C ABI bridge header pure C and import fixed-width types correctly.
- Define ownership and `defer` cleanup for every opaque handle.
- Set an explicit minimum macOS deployment target supported by RealityKit.
- Decide arm64-only versus universal arm64/x86_64 and test the chosen target.
- Make CMake produce an Xcode-compatible engine artifact or build the engine as
  an Xcode target; do not rely on a manually copied `.a` file.
- Add a smoke target that imports SwiftUI and RealityKit and links the ABI.

## Acceptance

`xcodebuild build` succeeds from a clean checkout after Step 12 passes.
