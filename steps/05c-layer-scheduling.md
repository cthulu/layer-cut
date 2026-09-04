STEP 5C — Layer scheduling and slicing orchestration
  Goal: Generate deterministic layers from normalized mesh contours.
  Action: engine/src/slicer.cpp + engine/include/slicer.h
  Tech:
    - Validate positive finite layer height and calculate layer count with a
      documented tolerance. Sample z = (i + 0.5) * layerHeight.
    - Never emit the top boundary as a duplicate zero-thickness layer.
    - Return contours plus each layer's Z, bounds, and diagnostics.
    - Test a normalized 20 mm cube at 0.2 mm: 100 non-empty layers, each with
      a 20 × 20 mm contour, within a stated numeric tolerance.
