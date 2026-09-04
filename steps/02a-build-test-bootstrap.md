STEP 2A — Build and test bootstrap
  Goal: Make every later engine step compilable and testable.
  Action: Root CMakeLists.txt, tests/CMakeLists.txt, and dependency wiring.
  Tech:
    - Pin and initialize Clipper2, CLI11, and stb_image_write as appropriate.
    - Define explicit engine source lists, include paths, C++17, warnings,
      development sanitizers, and CTest integration.
    - Add a minimal compiling engine target and one passing test executable.
    - Document native macOS and supported cross-platform build commands.
