STEP 18 — 3D viewport (model preview)
  Goal: Show the 3D model with a slice plane indicator.
  Action: macos-app/Viewport3DView.swift
  Tech:
    - Use SceneKit (built into macOS) — no external deps
    - Load STL via custom parser (reuse engine's STL loader)
    - Draw a horizontal plane at the current layer's Z position
    - Highlight active layer in preview panel when viewport interacted
    - Orbit camera: pan, zoom, rotate via SceneKit SCNNode gestures
    - AI prompt suggestion:
      "Create a SceneKit 3D viewport in SwiftUI that loads a binary
       STL and shows a movable slice plane"
