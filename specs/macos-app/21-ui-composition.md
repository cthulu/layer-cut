# STEP 21 - macOS UI Composition and Preview Modes

Priority: P0

## Goal

Define the visible composition of the Layer Cut macOS window after the UI cleanup.
The specification is intentionally separate from the implementation steps for the
parameter controls, inline SVG layer preview, and RealityKit renderer: those components
must compose into one predictable workflow.

## Window Composition

The primary window uses a three-region layout:

1. A left inspector/sidebar containing file import state, the active profile and
   slicing parameters, output selection, preview/export actions, progress, and
   status/error feedback.
2. A main preview workspace containing the original-model viewport and the
   optional stacked-layer viewport as two vertically stacked panes.
3. An optional layer-preview viewer inside the sidebar's `Preview` section,
   containing one generated SVG layer and its next-layer guide. It never opens
   a modal sheet or adds a workspace pane.

The sidebar remains usable while a preview or export is running. Long-running
operations show progress and provide cancellation where the service supports it.
The window must remain usable at its minimum supported size; panes may compress,
but controls must not overlap or become inaccessible.

## Stacked Panes Preview

The main workspace presents two equal-purpose panes in a horizontal split:

- Top/left pane: `Original STL model`, rendered from the engine mesh snapshot,
  with the active horizontal slicing plane.
- Bottom/right pane: `Stacked layer model`, rendered from the optional engine
  stacked-preview mesh. It represents physical layer thickness and ordering,
  not a second STL import.

The implementation may use a platform-appropriate split orientation, but the two
pane titles and roles must remain clear. Each pane has independent camera reset
behavior where practical; transform/profile changes invalidate and refresh both
representations together. If no stacked preview exists, the second pane shows a
specific empty/loading/error state inside the full assigned pane rather than
removing or collapsing the viewport. Generating the
stacked preview is controlled by the live-preview toggle and may be started on
demand from the sidebar.

## Triangle 3D Visualization

Both 3D panes use the RealityKit viewport contract from `18-3d-viewport.md`:

- Geometry comes from engine-provided vertex/index snapshots; Swift does not
  parse STL or reimplement topology.
- Coordinates are millimetres and use the active profile transform.
- The rendered object is visibly triangle-based (triangle mesh surface, with
  optional wireframe/edge treatment when available); it must not be represented
  as a 2D icon or a misleading solid placeholder.
- Orbit, pan, scroll zoom, axis presets, fine rotation, scale preview, and
  independent camera/transform reset remain available.
- The reset-camera help is collapsed into a compact control by default and can be
  expanded on demand.
- The active slicing plane includes a light-gray 5 mm grid for orientation; this
  is viewport-only and is never export geometry.
- The active layer is represented by a horizontal slice plane in the original
  pane. Selecting a layer in the layer preview updates that plane and the
  highlighted layer state.

The stacked pane uses the stacked mesh snapshot and must communicate that it is a
visualization of layer stacking. It must not be presented as export confirmation;
export completion and errors remain in the sidebar/status region.

## Layer Preview On Demand

The 2D layer preview is an explicit, inline sidebar surface:

- The sidebar exposes a `Layer preview` toggle, off by default.
- Enabling it triggers one complete SVG layer generation and shows determinate
  approximate progress plus a spinner; disabling it cancels/clears the work.
- The section shows a synchronized slider and numeric layer control above a
  bounded, full-width SVG viewer.
- The preview uses raw model coordinates, not Cricut page packing.
- The guide always comes from the immediate numeric next layer; an empty next
  layer produces no guide.
- The selected layer updates the original 3D pane but does not replace the
  stacked 3D pane.
- Layer-preview generation is not required merely to show the main 3D
  workspace. The user can inspect both 3D panes with the toggle off.
- All slicing-affecting model, profile, and transform changes reset the selected
  layer to 0 and regenerate only when the toggle is enabled.

## State and Accessibility Requirements

The UI must make these states distinguishable: no model loaded, model loaded but
not yet sliced, stacked preview loading, stacked preview unavailable/failed,
layer preview disabled/loading/ready/failed, export running, and export
completed/failed.
Use text labels in addition to color or icons. Buttons and panes need accessible
labels that describe their action and preview role.

## Related Specifications

- `14-file-import-ui.md` defines import and model metadata.
- `15-parameters-panel.md` defines profile and output controls.
- `17-layer-preview.md` defines inline layer SVG generation, guide geometry, and
  layer selection behavior.
- `18-3d-viewport.md` defines the RealityKit and mesh boundary.
- `19-export-workflow.md` defines export states and output semantics.
