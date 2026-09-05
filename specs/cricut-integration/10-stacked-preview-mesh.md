# Step 10 - Stacked Preview Mesh

Priority: P1

## Goal

Convert cleaned slice contours into an accurate watertight mesh representing the
stacked physical layers.

## Layer Volume

For layer `i`, use:

```text
z_min = layer.z - layer_height / 2
z_max = layer.z + layer_height / 2
```

Because slicing samples at midpoint Z and min-Z is normalized to zero, the first
layer occupies `[0, layer_height]`.

## Mesh Generation

For every non-empty layer:

- Triangulate the filled 2D contours.
- Generate bottom cap triangles.
- Generate top cap triangles.
- Generate outer side walls.
- Generate hole side walls.
- Preserve disconnected components.
- Use consistent triangle winding and normals.
- Preserve XY coordinates and exact layer thickness.

Reuse the same polygon union and manufacturing cleanup pipeline as cutting so the
preview matches exported geometry.

## Robustness

Reject non-finite coordinates and triangulation failures. Do not silently create
an approximate mesh. Report the layer index and contour responsible for failure.

## Acceptance

The output must represent concavities, holes, changing profiles, and disconnected
parts without triangles outside the intended contours.
