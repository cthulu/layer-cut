STEP 8 — PNG output generator (fallback)
  Goal: Export one physically sized PNG file per numbered layer.
  Action: engine/src/png_writer.cpp + engine/include/png_writer.h
  Tech:
    - Rasterize contours directly or select and test a declared rasterizer;
      stb_image_write only encodes the resulting pixel buffer.
    - Convert dimensions as ceil(canvas_mm / 25.4 * dpi), with documented
      bounds, antialiasing, fill rule, and background semantics.
    - Use stb_image_write (header-only, public domain) for encoding.
    - Black paths on white background (configurable)
    - Filename: layer_N.png with a deterministic zero-padding policy.
    - Embed the requested DPI in PNG resolution metadata and test pixel
      dimensions, filled pixels, holes, and physical-size interpretation.
