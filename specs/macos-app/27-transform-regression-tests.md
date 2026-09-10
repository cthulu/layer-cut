# STEP 27 - Multi-Axis Transform Regression Tests

Priority: P0

## Engine Tests

Use an asymmetric synthetic cuboid and verify:

- Default transform preserves baseline bounds and layer count.
- X, Y, and Z rotations independently change the expected bounds.
- Combined X/Y/Z rotations produce deterministic bounds and layer counts.
- Snapshot geometry and sliced geometry use the same transform.
- All transform validation limits reject invalid values.

## CLI Tests

Run the CLI with explicit cutting-axis and X/Y/Z rotation values. Verify that
the generated SVG or stacked STL differs from the identity output and that
repeating the command produces byte-stable output.

## macOS Tests

Verify that:

- Transform values are not included in profile serialization.
- Opening a new STL resets the session transform.
- Reset transform restores the documented defaults.
- Changing any transform refreshes both viewport and layer previews.
- Export uses the same transform shown in the viewport.

## Known Limitation

The custom stacked-preview triangulator may reject complex contours. Such
failures must remain diagnostic and must not silently produce an empty STL.
