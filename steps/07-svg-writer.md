STEP 7 — SVG output generator
  Goal: Export sliced layers as SVG files.
  Action: engine/src/svg_writer.cpp + engine/include/svg_writer.h
  Tech:
    - Generate SVG <path> elements per polygon per layer
    - Each layer gets a <g> group with layer index metadata
    - Stroke width configurable (default 0.5mm → scaled to viewBox)
    - Color per layer for visual distinction
    - Optional: cut marks, registration marks
    - Unit test: Generate SVG, verify valid XML & path data
