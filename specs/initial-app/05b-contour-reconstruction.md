STEP 5B — Segment deduplication and contour reconstruction
  Goal: Turn plane segments into oriented closed contours.
  Action: engine/src/contour_builder.cpp + headers/tests.
  Tech:
    - Snap endpoints within a documented tolerance using spatial hashing.
    - Deduplicate shared edges and remove zero-length segments.
    - Build loops from an adjacency graph; never globally sort points or use
      ear-clipping for reconstruction.
    - Detect dangling edges and ambiguous branching and return diagnostics.
    - Classify outer contours and holes by signed area and normalize winding.
    - Test concave shapes, holes, disconnected solids, touching components,
      and multiple contours in one layer.
