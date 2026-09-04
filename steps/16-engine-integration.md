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
    - Error handling: throw Swift errors for C-level failures
    - Progress reporting: callback or async sequence
    - Memory management: ensure slicer_free_* called in defer blocks
