# Step 14 - Tight Packing and Orthogonal Rotation

Priority: P3

## Goal

Add `tight-packing` as an optional packing strategy after fixed packing is stable.
Tight packing may rotate individual layers by deterministic orthogonal angles to
improve page utilization.

## Requirements

- Keep fixed packing behavior unchanged.
- Compute actual cleaned layer bounds.
- Preserve deterministic placement.
- Evaluate only the orientations `0`, `90`, `180`, and `270` degrees.
- Rotate the complete layer geometry, including disconnected contours and holes,
  around the layer rectangle's center before placement.
- Respect page borders and usable dimensions.
- Respect positive and negative gaps.
- Respect the 4,500-path budget.
- Keep cut and guide placement consistent with the selected layer orientation.
- Expose the strategy through the CLI and profile UI only when implemented.
- Keep `fixed` packing as the default and preserve its no-rotation behavior.

## Algorithm

Use a deterministic shelf, guillotine, or equivalent rectangle-packing algorithm.
The implementation must document the selected algorithm, tie-breaking, ordering,
coordinate rounding, and failure behavior.

For each layer, derive a rectangle for every allowed orientation, including the
configured gap in the placement footprint. Place layers in ascending layer index
order. For equal utilization, choose the lowest page index, then lowest Y, then
lowest X, then orientation order `0`, `90`, `180`, `270`. Any remaining ties are
resolved by ascending layer index.

The rotation is part of the page model and must be applied identically to cut
geometry, guide geometry, layer metadata, and emitted bounds. Coordinates are
rounded only at final SVG serialization using the existing SVG rounding policy.
If no allowed orientation fits the usable rectangle, reject the layer instead of
scaling or splitting it.

## Tests

- Compare fixed and tight output for layers with different bounds. Assert that
  geometry is unchanged and only placement/page utilization differs.
- Use a fixture where a 90-degree rotation improves utilization or reduces page
  count.
- Assert deterministic output across repeated runs and stable orientation choice.
- Assert holes, disconnected contours, guides, metadata, borders, negative-gap
  warnings, and path budgets remain correct after rotation.
- Assert fixed packing remains byte-stable and never rotates layers.
