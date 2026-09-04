STEP 5 — Slicing engine (layer generation)
  Goal: Convert 3D mesh into 2D cross-sections at layer heights.
  Action: engine/src/slicer.cpp + engine/include/slicer.h
  Tech:
    - For each layer z = base + i × layerHeight:
      find all triangles intersecting the plane
    - For each triangle, check if vertices straddle the layer plane
    - Compute 2D intersection points on the layer plane
    - Output: std::vector<std::vector<Vec2>> per layer
    - Sort points to form closed polygons (ear-clipping for concave)
    - Unit test: Cube at 0.2mm → 100 layers, each a 20×20mm square
