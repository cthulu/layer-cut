# Step 14 - Tight Packing

Priority: P3

## Goal

Add `tight-packing` as an optional packing strategy after fixed packing is stable.

## Requirements

- Keep fixed packing behavior unchanged.
- Do not rotate layers initially.
- Compute actual cleaned layer bounds.
- Preserve deterministic placement.
- Respect page borders and usable dimensions.
- Respect positive and negative gaps.
- Respect the 4,500-path budget.
- Keep cut and guide placement consistent.
- Expose the strategy through the CLI only when implemented.

## Algorithm

Use a deterministic shelf, guillotine, or equivalent rectangle-packing algorithm.
Document tie-breaking, ordering, coordinate rounding, and failure behavior.

## Tests

Compare fixed and tight output for layers with different bounds. Assert that
geometry is unchanged and only placement/page utilization differs.
