# STEP 23 - Independent Cutting Orientation and Rotation

Priority: P0

## Goal

Separate the axis used for cutting from the user-controlled rotations so a model
can be rotated independently around X, Y, and Z before slicing.

## Transform Contract

The engine must compose transforms in this fixed order:

```text
final = scale * cutting_axis_orientation * rotate_z * rotate_y * rotate_x * input
```

- `cutting_axis_orientation` maps the selected model axis to the slicer's Z axis.
- `rotate_x`, `rotate_y`, and `rotate_z` are independent Euler rotations in the
  cutting frame.
- Angles are degrees and each is limited to `-180` through `180`.
- Scale remains positive and is applied uniformly.
- The transformed mesh is normalized by translating its minimum Z to zero before
  slicing.
- The same composed transform must drive snapshots, layers, pages, PNGs, and
  stacked STL output.

## Defaults

- Cutting axis: `+Z`.
- Rotation X/Y/Z: `0` degrees.
- Scale: `1`.

The camera orbit is presentation-only and is not part of this transform.
