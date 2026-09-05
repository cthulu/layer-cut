# STEP 11 - macOS App Architecture Decision

Priority: P0

## Goal

Define the macOS-first frontend boundary before creating UI files.

## Decision

- Use SwiftUI for the macOS application UI.
- Use RealityKit for the interactive 3D viewport.
- Do not use SceneKit for new code.
- Keep the C++17 engine as the geometry authority.
- Use the C ABI for Swift interoperability.
- Keep profiles, transforms, exports, diagnostics, and mesh snapshots platform-neutral.
- Use YAML for persisted profiles.

## Required Engine Boundaries

The frontend must not parse STL independently. The engine/C ABI must provide:

- Validated mesh metadata and normalized bounds.
- A normalized/transformed mesh snapshot for RealityKit.
- Transform configuration applied before slicing.
- Layer SVG/PNG results.
- Cricut cut/guide page results.
- Stacked STL output or an export result describing it.
- Diagnostics, progress, and cancellation contracts.

## Transform Contract

- Store orientation as a transform, preferably quaternion or matrix internally.
- Provide axis presets for `+X`, `-X`, `+Y`, `-Y`, `+Z`, and `-Z`.
- Provide fine rotation around the selected up axis.
- Treat scale as an export transform, not a display-only scale.
- Apply the complete transform before normalization and slicing.
- Persist the transform in the active profile.

## Portability Contract

Linux is not a first-release frontend target. Do not introduce Swift-specific
semantics into engine behavior. A future Linux UI must consume the same C ABI
and profile schema.
