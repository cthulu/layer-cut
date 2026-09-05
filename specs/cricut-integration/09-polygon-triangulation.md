# Step 09 - Polygon Triangulation

Priority: P1

## Goal

Provide deterministic 2D triangulation for accurate stacked STL generation.

## Requirements

Support:

- Concave outer contours.
- Holes.
- Multiple disconnected components.
- Stable winding and deterministic triangle order.
- Degenerate and near-zero-area contours.
- Finite coordinates only.

Do not use a naive triangle fan because it produces triangles outside concave
boundaries.

## Dependency Decision

First inspect the pinned Clipper2 dependency for suitable triangulation support.
If unavailable, use a pinned, appropriately licensed triangulation implementation
or add a carefully tested internal implementation with hole bridging.

Document the selected algorithm, license, coordinate tolerances, winding rules,
failure behavior, and memory limits.

## Tests

Test convex, concave, holed, disconnected, degenerate, and near-touching inputs.
Assert that every triangle lies within the intended filled region.
