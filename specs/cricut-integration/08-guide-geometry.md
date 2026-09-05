# Step 08 - Production Guide Geometry

Priority: P1

## Goal

Generate a pen-drawn outline of the next slice on the previous slice without
adding material, holes, tabs, or sacrificial geometry.

## Geometry

- Use a true inward polygon offset in millimetres.
- Default inset is 1 mm and must be configurable.
- Apply the offset to all relevant outer contours and holes.
- Preserve disconnected components.
- Reject or omit self-intersecting offset results.
- Validate that the guide is contained by the previous cleaned layer.
- Omit the guide and emit a warning when containment fails.
- Never silently substitute uniform scaling.

## Edge Cases

Define behavior for:

- Empty previous layer.
- Empty next layer.
- Concave outlines.
- Holes.
- Thin features that collapse under offset.
- Offset results with self-intersections.
- Multiple disconnected components.
- Guide paths exceeding the page path budget.

## Cricut Semantics

The guide SVG is still imported as Cut by default. Documentation must instruct
the user to select the named guide group, change it to `Draw > Pen`, and attach
it to the corresponding cut layer before plotting.

The guide page also includes the shared alignment marker/registration geometry
defined by the page model. Its shape and page coordinates must be identical to
the marker in the paired cut SVG so the two separately imported documents can be
stacked correctly in Design Space. The marker must not be treated as guide
geometry or assigned to `Draw > Pen`.
