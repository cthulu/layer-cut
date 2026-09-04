STEP 6 — 2D boolean operations (Clipper2)
  Goal: Merge overlapping polygons per layer into clean outlines.
  Action: engine/src/polygon_ops.cpp + engine/include/polygon_ops.h
  Tech:
    - Link Clipper2 library (Zlib license, no AGPL concern)
    - Operations: Union, Difference, Intersection of 2D polygons
    - Per layer: union all polygons → single/multiple closed outlines
    - Handle nested contours (holes) natively
    - Unit test: Two overlapping circles → single merged polygon
