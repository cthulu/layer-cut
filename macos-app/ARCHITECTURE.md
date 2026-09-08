# macOS App Architecture

This document is the implementation contract for the macOS frontend. It keeps
the UI replaceable and prevents Swift-specific behavior from entering the
geometry engine.

## Frontend Boundary

- SwiftUI owns windows, panels, profile editing, file selection, and export
  commands.
- RealityKit owns the interactive 3D viewport.
- The viewport uses a Cura-like orbit camera: the default is a three-quarter
  view showing the top and two side faces. Press and hold the secondary (right)
  mouse button and drag to orbit around the camera target; horizontal movement
  changes yaw and vertical movement changes clamped pitch. The middle button
  pans, and the scroll wheel zooms. These controls are presentation-only.
- Primary-button gestures remain available for selection and future direct
  manipulation; orbiting must not depend on a primary-button drag.
- The frontend does not parse STL files or calculate mesh geometry itself.
- The frontend communicates with the engine only through `engine/include/layer_cut.h`.
- SceneKit is not used for new code.

## Engine Boundary

The C++17 engine remains the authority for:

- STL loading, validation, metadata, and normalized bounds.
- Transform application before normalization and slicing.
- Mesh snapshots consumed by RealityKit.
- Layer SVG/PNG generation and Cricut page generation.
- Stacked STL export.
- Diagnostics, progress, and cancellation.

The viewport interaction state is UI state. It controls the RealityKit camera
only and never changes the engine transform or exported geometry.

Opaque C handles own engine resources. Swift bindings must release every mesh,
configuration, and result handle, including failure paths.

## Transform Contract

Profiles persist an orientation transform, fine rotation, and export scale.
The engine exposes the six up-axis presets `+X`, `-X`, `+Y`, `-Y`, `+Z`, and
`-Z`. The complete transform is applied before normalization and slicing, so
the viewport and exported layers describe the same geometry.

Scale is an export transform, not a display-only adjustment.

## Profile and Portability Contract

Profiles are persisted as YAML. Profile fields, transforms, diagnostics, mesh
snapshots, and export results remain platform-neutral. Linux is not a first
release frontend target, but a future frontend must be able to consume the
same C ABI and profile schema without changing engine behavior.
