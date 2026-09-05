STEP 12 — Test STL model
  Goal: Provide a known test model for verification.
  Action: tests/assets/cube.stl (binary STL)
  Tech:
    - Simple 20mm × 20mm × 20mm cube
    - Generate with Python script or Blender
    - After min-Z normalization, expected: 100 layers at 0.2mm, each a
      20×20mm square. Assert contour coordinates and areas with tolerance.
    - Require a geometry fixture suite covering holes, concavity, tilted faces,
      disconnected solids, negative coordinates, malformed input, unsafe
      topology warnings, and non-divisible heights.
