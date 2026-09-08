# STEP 18 - RealityKit 3D Viewport

Priority: P0

## Goal

Show the transformed mesh, orientation, scale, and active slicing plane.

## Requirements

- Use SwiftUI with `RealityView` or an equivalent RealityKit container.
- Build the displayed mesh from the engine-provided mesh snapshot.
- Do not parse STL or use a second topology implementation in Swift.
- Use a stable world-coordinate contract in millimetres.
- Support orbit, pan, zoom, axis presets, fine rotation, and scale preview.
- Draw a horizontal slice plane at the active layer Z.
- Draw a light-gray 5 mm by 5 mm viewport-only grid on the active slice plane.
- Link layer selection and viewport highlighting.
- Reset camera and transform independently.
- Keep viewport transforms synchronized with the active profile transform.
- Fill the assigned pane even when no snapshot exists; show empty, loading, or
  error state inside the pane.
- Start in a documented Cura-like three-quarter camera orientation and orbit
  around the camera target without changing engine transforms.
- Keep reset-camera and interaction help available through a collapsible compact
  control.

## Rendering Boundary

Keep RealityKit-specific mesh conversion in a Swift adapter. The C ABI exposes
plain vertex/index data only; it must not expose Swift, RealityKit, or Metal types.

The grid and camera controls are presentation-only. They must not be included in
engine snapshots or any exported artifact.
