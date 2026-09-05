STEP 10 — CLI tool implementation
  Goal: Command-line slicer:
    layer-cut model.stl -o output/ -l 0.2
  Action: cli/main.cpp
  Tech:
    - Argument parsing: CLI11 (header-only, BSD-3)
    - Use one consistent input syntax. Define flags for --output-dir,
      --layer-height, --dpi, --format (svg|png), and optional canvas bounds.
    - State the millimetre assumption, Z/XY convention, and min-Z
      normalization in --help.
    - Default layer height: 0.2mm, default format: SVG
    - Flow: load STL → slice → write SVG/PNG → print summary
    - Exit codes: 0 = success, 1 = error, 2 = invalid input
    - Unit test: Run CLI with test STL, verify output files
