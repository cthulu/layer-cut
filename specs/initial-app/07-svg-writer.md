STEP 7 — SVG output generator
  Goal: Export one physically sized SVG file per numbered layer.
  Action: engine/src/svg_writer.cpp + engine/include/svg_writer.h
  Tech:
    - Generate SVG <path> elements per polygon per layer
    - Each layer gets a <g> group with layer index metadata
    - Use filled paths with `fill-rule="evenodd"`; omit stroke by default
      because stroke changes the physical cut boundary.
    - Use a millimetre viewBox matching explicit output bounds and document
      origin and Y-axis orientation.
    - Keep visual colors optional and separate from cut geometry
    - Optional: cut marks, registration marks
    - Unit test: Verify XML, physical millimetre viewBox, path closure, fill rule, holes,
      bounds, and coordinates against known geometry.
