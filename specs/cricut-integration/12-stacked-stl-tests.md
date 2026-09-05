# Step 12 - Stacked STL Tests

Priority: P1

## Fixtures

Use or add fixtures for:

- 20 mm cube.
- Concave prism.
- Prism with a hole.
- Disconnected solids.
- Layer profiles that expand and contract.
- Non-divisible model height.
- Empty intermediate layers.

## Assertions

- Minimum and maximum Z match the scheduled stack.
- Each layer has the configured thickness.
- XY bounds match cleaned contours.
- No NaN or infinite coordinates exist.
- Triangle winding and normals are consistent.
- Holes and concavities are preserved.
- Disconnected components remain disconnected.
- Output is deterministic.
- Generated ASCII STL can be parsed again.

Add CLI integration coverage for `--stacked-stl` alongside existing export tests.
