STEP 6 — 2D boolean operations (Clipper2)
  Goal: Merge overlapping polygons per layer into clean outlines.
  Action: engine/src/polygon_ops.cpp + engine/include/polygon_ops.h
  Tech:
    - Link Clipper2 library (Zlib license, no AGPL concern)
    - Use a documented fixed integer scale, rounding mode, coordinate bounds,
      tolerance, fill rule, and winding policy. Reject values that overflow
      the selected integer range rather than silently reducing precision.
    - Operations: Union, Difference, Intersection of 2D polygons
    - Per layer: union reconstructed contours while preserving multiple
      solids and holes; never use this step to repair open contours.
    - Unit test: overlapping, nested, disjoint, and near-coincident polygons.
