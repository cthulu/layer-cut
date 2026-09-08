# STEP 22 - UI Refinements and Viewport Presentation

Priority: P1

## Goal

Improve the macOS application's initial window, viewport sizing, export feedback,
and 3D interaction without moving engine responsibilities into SwiftUI.

## Window

- The primary application window opens maximized on launch.
- Maximization must use the platform windowing API rather than a hard-coded frame
  or screen size.
- Preserve the minimum supported window size and allow normal restore and resize.

## Viewport Pane Occupancy

- Both assigned 3D viewport panes must fill their complete pane bounds whether or
  not a mesh snapshot is available.
- An empty, loading, or unavailable state must be rendered inside the viewport
  pane, not by removing the viewport or allowing it to collapse to intrinsic size.
- The original and stacked panes retain their existing independent roles and
  engine snapshot boundary.

## Export Warnings

- Do not render export warnings directly as an always-visible list in the sidebar.
- When warnings or diagnostics exist, show a clearly labelled button containing a
  count or warning indicator.
- Activating the button opens a modal dialog listing warning and error messages in
  a selectable, scrollable, accessible form.
- The dialog must distinguish warnings from errors using text as well as styling.
- Export completion, partial failure, and fatal export errors remain visible in the
  normal status/error region.

## Camera Controls

- The reset-camera overlay is collapsed by default to a compact button.
- Activating that button reveals interaction help, reset-camera, reset-transform,
  and layer controls without covering unnecessary viewport area.
- The control must remain keyboard- and VoiceOver-accessible.

## Cutting-Plane Grid

- Draw a light-gray grid on the active cutting plane at 5 mm by 5 mm spacing.
- The grid is viewport-only presentation geometry and must never be included in
  exported SVG, PNG, STL, or engine geometry.
- The grid follows the active plane bounds and active layer Z, updating when the
  profile transform or selected layer changes.
- The grid remains visually subordinate to the model and cutting plane.

## Cura-Like Camera Contract

- Use an explicit, documented default camera orientation comparable to Ultimaker
  Cura: a three-quarter view with the model's top and two side faces visible.
- Orbit around the model/camera target, preserving pan and scroll zoom.
- Use consistent horizontal yaw and vertical pitch behavior with a clamped pitch;
  camera rotation must not modify engine transforms or exported geometry.
- Keep the interaction mapping documented in viewport help and consistent across
  both viewports.

## Tests and Acceptance

- Add UI or platform acceptance checks for maximized startup, full-pane empty
  states, warning-dialog presentation, and collapsed camera controls.
- Add viewport checks for 5 mm grid spacing, light-gray appearance, grid exclusion
  from exports, default camera orientation, and orbit behavior.
- Verify all derived geometry and placement values continue to come from engine
  snapshots or export results.
