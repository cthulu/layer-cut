STEP 18 — 3D viewport (model preview)
  Goal: Show the 3D model with a slice plane indicator.
  Action: macos-app/Viewport3DView.swift
  Tech:
    - Use SceneKit (built into macOS) — no external deps
    - Load the validated/normalized mesh through the engine boundary; avoid a
      second Swift parser with different unit or topology behavior.
    - Draw a horizontal plane at the current layer's Z position
    - Highlight active layer in preview panel when viewport interacted
    - Orbit camera: pan, zoom, rotate via SceneKit SCNNode gestures
    - AI prompt suggestion:
      "Create a SceneKit 3D viewport in SwiftUI that loads a binary
       STL and shows a movable slice plane"
