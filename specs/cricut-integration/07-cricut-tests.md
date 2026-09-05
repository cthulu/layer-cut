# Step 07 - Cricut Export Tests

Priority: P0

## Unit Tests

Cover:

- Normal page dimensions.
- Large page dimensions.
- Seven millimetre borders.
- Row-major ascending placement.
- Page splitting.
- Shared cut and guide transforms.
- Guide placement in the previous layer's tile.
- Positive gaps.
- Negative gaps and overlap warnings.
- Oversized layer rejection.
- 4,500-path budget enforcement.
- Empty layers.
- Concave contours.
- Holes.
- Disconnected components.
- Multiple paths in one layer.
- Stable filenames and output ordering.

## Integration Tests

- Generate the five-layer sample from the existing `output` fixture.
- Generate a multi-page output from a synthetic layer set.
- Verify cut and guide SVGs have equal page dimensions.
- Verify all emitted path counts.
- Verify combined SVGs contain `cut-layers` and `pen-layers` groups and no
  alignment marker.
- Verify existing `svg` and `png` CLI tests remain unchanged.
- Do not add automated Design Space validation; Cricut application behavior is
  recorded through the manual validation artifact only.

## Acceptance

Run the normal CTest command and inspect generated SVGs for deterministic group
names, transforms, dimensions, and path counts.
