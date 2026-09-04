STEP 8 — PNG output generator (fallback)
  Goal: Export sliced layers as 300 DPI PNG images.
  Action: engine/src/png_writer.cpp + engine/include/png_writer.h
  Tech:
    - Use stb_image_write (header-only, public domain)
    - Render SVG paths to raster at 300 DPI
    - Black paths on white background (configurable)
    - Filename: layer_N.png
    - Unit test: Verify pixel dimensions match 300 DPI
