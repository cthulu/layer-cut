# STEP 17 - Layer Preview Panel

Priority: P1

## Goal

Show generated layers and connect selection to the 3D viewport.

## Requirements

- Use a SwiftUI `ScrollView` and lazy grid/list.
- Request engine-generated PNG preview bytes at UI-appropriate DPI.
- Render copied PNG data with `Image`/`NSImage` without an SVG renderer.
- Show layer index, Z height, empty status, and warnings.
- Select a layer to highlight its plane in RealityKit.
- Support zoom and clear loading/error states.
- Show page membership and output format where relevant.
