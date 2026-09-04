STEP 5A — Triangle-plane intersection primitives
  Goal: Correctly convert individual triangles into plane segments.
  Action: engine/src/plane_intersection.cpp + headers/tests.
  Tech:
    - Define epsilon rules for below/on/above-plane vertices.
    - Handle vertex touches, edges on the plane, and fully coplanar triangles
      without zero-length or duplicate segments.
    - Return 2D XY segments with enough identity to deduplicate shared edges.
    - Test every sign/topology case and numerical boundary case.
