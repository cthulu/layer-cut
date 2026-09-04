STEP 4A — Mesh validation and normalization
  Goal: Establish a safe, normalized solid before slicing.
  Action: engine/src/mesh_validation.cpp + tests.
  Tech:
    - Reject empty and non-finite input. Detect degenerate, open,
      non-manifold, and self-intersecting input, but continue best-effort where
      possible.
    - Emit structured warnings identifying unsafe topology and affected
      operations; never claim that best-effort output is a valid solid.
    - Compute bounds and translate minZ to zero without changing X/Y.
    - Return a structured diagnostic containing category and useful values.
    - Test negative coordinates, duplicate vertices, inverted winding,
      disconnected closed components, and invalid topology.
