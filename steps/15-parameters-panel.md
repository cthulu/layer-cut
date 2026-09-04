STEP 15 — Slicing parameters panel
  Goal: Configurable layer height, model dimensions, output format.
  Action: macos-app/ParametersPanel.swift
  Tech:
    - Sliders/steppers: layer height (0.1–0.3mm, step 0.01)
    - Show model X/Y/Z bounds read-only after normalization. Distinguish
      those from optional output canvas width/height and never silently scale.
    - Segmented control: output format (SVG / PNG)
    - "Slice" button triggers engine call
    - Show estimated layer count and output file count
