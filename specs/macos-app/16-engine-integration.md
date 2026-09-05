STEP 16 — Engine integration (Swift ↔ C++)
  Goal: Call the C++ slicer from Swift, receive SVG/PNG paths.
  Action: macos-app/SlicingService.swift
  Tech:
    class SlicingService {
        func slice(stlPath: String, layerHeight: Float,
                   width: Float, height: Float)
            throws -> [LayerOutput]
        // Calls: slicer_load_stl → slicer_slice →
        //        slicer_result_layer_svg → collect → free
    }
    - Error handling: throw Swift errors from C status codes and retrievable
      diagnostics, not from null pointers alone.
    - Progress reporting and cancellation must be implemented in the C ABI
      before the UI exposes them, or explicitly deferred. Never block the
      main actor during slicing.
    - Memory management: ensure slicer_free_* called in defer blocks
