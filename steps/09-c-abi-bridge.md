STEP 9 — C ABI bridge header
  Goal: Expose engine functionality via a clean C interface for Swift/FFI.
  Action: engine/include/cricut_slicer.h
  Tech:
    typedef void* slicer_mesh_t;
    typedef void* slicer_result_t;

    slicer_mesh_t slicer_load_stl(const char* path);
    slicer_result_t slicer_slice(slicer_mesh_t mesh,
                                 float layer_height_mm,
                                 float width_mm, float height_mm);
    int slicer_result_layer_count(slicer_result_t result);
    const char* slicer_result_layer_svg(slicer_result_t result, int idx);
    const char* slicer_result_layer_png_path(slicer_result_t result, int idx);
    void slicer_free_mesh(slicer_mesh_t mesh);
    void slicer_free_result(slicer_result_t result);

    - All memory managed by engine (caller frees via *_free_*)
    - No C++ types in the header — pure C
    - Unit test: Call from a C program, verify correct output
