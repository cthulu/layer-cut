# STEP 17 - Inline Layer Preview

Priority: P1

## Goal

Replace the on-demand layer-preview sheet with an inline 2D preview that shows
one cut layer and the outline used to position the next layer. The feature is
explicitly opt-in because generating preview geometry can be expensive.

## Preview Panel

- Remove the `Preview layers` button.
- Remove the layer-preview sheet/modal and its layer thumbnail grid.
- Add a `Layer preview` toggle to the existing `Preview` section.
- The toggle is off by default and is not persisted in application settings.
- Add a human-friendly tooltip/help text explaining that enabling it generates
  an SVG preview and may take time.
- Keep `Live preview` and `Layer preview` independent. Live preview controls
  the stacked 3D viewport; Layer preview controls the 2D SVG viewport.

## Inline 2D View

When `Layer preview` is enabled, the existing sidebar `Preview` section expands
to show the layer preview. The preview remains inside the sidebar and never
opens a modal window or adds a main-workspace pane.

The bounded, full-width viewer displays:

- The selected layer's cut geometry as SVG.
- The next layer's guide outline over the selected layer, using the same
  alignment semantics as a combined Cricut SVG.
- The current layer number and total layer count.
- A loading state with a spinner while the SVG is being generated.
- A clear empty state when no model is loaded or no layers are available.
- A clear error state with the engine error and a retry action where practical.

The SVG renderer remains a presentation boundary. SwiftUI must not parse STL,
reconstruct contours, or implement polygon offsets.

## Layer Selection

Place the layer selector above the viewer in the sidebar:

- Slider range is `0 ... layerCount - 1`.
- Numeric input shows the human-facing layer number (`1 ... layerCount`),
  while the engine/API continues to use a zero-based index.
- Both controls update the same selected-layer state and clamp invalid input.
- Disable both controls until a model has a known positive layer count.
- Selecting another layer requests the corresponding preview. If the engine
  returns a complete cached layer set, selection should update immediately.
- The selected layer also updates the active layer in the original 3D viewport.
- The final layer has no next-layer guide; show only its cut geometry and do not
  treat the missing guide as an error.
- Use a bounded fixed-height viewer so the preview does not make the parameter
  panel unusably tall. The exact height is an implementation detail within the
  supported minimum window size.

The existing `Stepper`-style layer control may be retained in the 3D camera
controls, but it must stay synchronized with this control. Do not introduce
independent layer selections.

## Geometry Semantics

For selected layer `N`:

- Render layer `N`'s cleaned cut geometry.
- If layer `N + 1` exists, render the inward-offset outline of layer `N + 1`
  over layer `N`, in the coordinate system and alignment position of layer `N`.
- Use the active profile's `guideInset` value in millimetres.
- Apply the same containment, offset, fallback, and warning rules as the
  combined Cricut SVG. In particular, an invalid or non-contained inset uses
  the next layer's un-inset outline with a warning, rather than silently
  dropping the guide.
- Preserve holes and disconnected components according to the engine's guide
  geometry contract.
- The preview is informational and must not add registration markers, page
  packing, layer numbers, or export-only artwork unless explicitly required to
  explain alignment.

The preview should be visually distinguishable: cut geometry uses the cut
appearance and the next-layer guide uses the guide appearance (outline/stroke).
The exact colors are presentation details, but meaning must not depend on color
alone.

## Generation and State

- Do not generate layer-preview SVGs while the toggle is off.
- Toggling on starts generation for layer 0 and shows the spinner immediately.
- Use the engine's determinate layer progress where available, while retaining
  a spinner and treating the percentage as approximate across non-layer phases.
- Toggling off cancels an in-flight request when safe, clears the displayed
  layer-preview SVG, and releases preview-only data where practical.
- A profile/parameter change invalidates the current layer-preview result. If
  the toggle is on, cancel the old request, set the selected layer to 0, and
  start a new request for layer 0.
- A model reload invalidates the current result. If the toggle is on, set the
  selected layer to 0 and start a new request after the model identity is
  available.
- Transform changes are parameter changes for this purpose.
- Stale asynchronous results must not replace a preview for a newer model,
  profile, transform, inset, or selected-layer request. Use an identity/generation
  token and cancellation at the Swift service boundary.
- If the toggle is enabled with no model loaded, do not invoke the engine;
  display the no-model state. Loading a model while the toggle remains enabled
  starts layer 0 generation.
- A failed request leaves the toggle enabled, clears stale geometry, shows the
  error state, and permits retry without requiring the user to toggle twice.

## Engine/API Investigation

The initial implementation reuses the complete slice result: generate all
layers once when the toggle is enabled, cache the result for the current
identity, and make layer selection immediate. The current `slicer_slice` API
already generates every layer and has no request for a single layer.

As a potential improvement, benchmark representative models before adding a
targeted engine path. Candidate improvements are:

1. Add an engine/C-ABI operation that generates the selected layer plus its next
   layer, then returns the composed preview SVG and warnings. This can reduce
   work for large models but may regenerate geometry while the slider moves.
2. Add a reusable sliced intermediate representation so the engine can retain
   one slice result and compose only the requested layer/guide on demand.

The preferred design is the smallest engine-owned API that avoids Swift-side
geometry logic. The API must accept the active transform, layer height,
cleanup settings, and `guideInset`; it must return copied SVG data, layer
metadata, warnings, and cancellation/progress support. It must not depend on
the selected export format, because layer preview is always SVG.

The benchmark/decision must cover:

- Initial generation time.
- Time to change layers.
- Peak memory for a complete result versus a targeted result.
- Cancellation responsiveness.
- Geometry equivalence with the existing combined SVG guide for fixed and tight
  packing profiles.

Neither targeted approach is part of the initial implementation. If a future
benchmark shows a meaningful improvement, add it behind the same service
boundary without changing the UI contract.

## Acceptance Criteria

- The old button, sheet presentation, thumbnail grid, and related state are
  removed.
- A fresh app session shows `Layer preview` off and performs no layer-preview
  generation until enabled.
- Enabling the toggle shows a spinner, then displays layer 0's SVG and the
  inset next-layer guide.
- Layer selection works through both slider and numeric input and remains
  synchronized with the original 3D viewport.
- The last layer displays without a guide and without an error.
- Changing the model, transform, profile, layer height, cleanup setting, or
  guide inset while enabled returns selection to layer 0 and regenerates.
- Disabling the toggle cancels/clears generation and display.
- Invalid inset/guide geometry follows the existing combined-SVG fallback and
  warning behavior.
- No stale asynchronous result can overwrite a newer preview.
- Unit/integration tests cover layer selection bounds, lifecycle resets,
  cancellation/stale results, last-layer behavior, inset/fallback behavior,
  and SVG visual/structural composition.

## Related Specifications

- `15-parameters-panel.md` defines profile and parameter controls.
- `16-engine-integration.md` defines the Swift/C ABI service boundary.
- `18-3d-viewport.md` defines active-layer synchronization.
- `21-ui-composition.md` defines the main-window composition and preview modes.
- `cricut-integration/08-guide-geometry.md` defines guide offset semantics.
- `cricut-integration/16-guide-outline-invariant.md` defines fallback behavior.
