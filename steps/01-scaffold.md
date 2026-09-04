LAYER-CUT — Technical Implementation Plan

Architecture Overview

  layer-cut/
  ├── engine/              # C++ core library (static .a)
  │   ├── include/         # Public C ABI header
  │   ├── src/             # Implementation
  │   └── CMakeLists.txt
  ├── cli/                 # Phase 1: CLI tool (links engine)
  │   └── main.cpp
  ├── macos-app/           # Phase 2: SwiftUI app (links engine)
   │   └── LayerCutApp.swift
  ├── devcontainer/        # DevContainer config
  │   ├── Dockerfile
  │   └── devcontainer.json
  └── tests/               # Unit + integration tests

STEP 1 — Create project scaffold
  Goal: Empty repo with folder structure.
   Action: Create layer-cut/ with subfolders.
  Tech: Standard folder creation. No code yet.
