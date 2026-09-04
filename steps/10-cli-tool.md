STEP 10 — CLI tool implementation
  Goal: Command-line slicer:
    cricut-slicer model.stl -o output/ -l 0.2
  Action: cli/main.cpp
  Tech:
    - Argument parsing: CLI11 (header-only, BSD-3)
    - Flags: --input, --output-dir, --layer-height,
              --width, --height, --format (svg|png)
    - Default layer height: 0.2mm, default format: SVG
    - Flow: load STL → slice → write SVG/PNG → print summary
    - Exit codes: 0 = success, 1 = error, 2 = invalid input
    - Unit test: Run CLI with test STL, verify output files
