# STEP 24 - Transform API and CLI Controls

Priority: P0

## C ABI

Add a transform entry point that accepts cutting axis, three rotation angles, and
scale. Mesh snapshots must accept the same values and produce identical geometry
to slicing.

The API must validate axis values, finite angles, angle limits, and positive
finite scale. Invalid input must set the existing thread-local diagnostic.

The old single-angle entry point may remain as a compatibility wrapper mapping
its angle to `rotate_z`, but new clients must use the multi-axis entry point.

## CLI

Expose explicit options:

```text
--cutting-axis +Z
--rotate-x 0
--rotate-y 0
--rotate-z 0
--scale 1
```

- All values are in degrees except scale.
- Defaults match the engine defaults.
- CLI help must describe the fixed transform order.
- Exported output and optional stacked STL must use these exact values.
- Invalid values must produce a command-line error before loading the STL.
