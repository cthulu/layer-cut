STEP 9 — C ABI bridge header
  Goal: Expose engine functionality via a clean C interface for Swift/FFI.
  Action: engine/include/cricut_slicer.h
  Tech:
    #include <stddef.h>
    #include <stdint.h>

    typedef void* slicer_mesh_t;
    typedef void* slicer_result_t;
    typedef void* slicer_config_t;

    slicer_mesh_t slicer_load_stl(const char* path);
    slicer_config_t slicer_config_create(void);
    int slicer_config_set_layer_height(slicer_config_t, double mm);
    int slicer_config_set_canvas(slicer_config_t, double width_mm,
                                 double height_mm);
    slicer_result_t slicer_slice(slicer_mesh_t mesh, slicer_config_t config);
    int slicer_result_layer_count(slicer_result_t result);
    const char* slicer_result_layer_svg(slicer_result_t result, int idx);
    const uint8_t* slicer_result_layer_png(slicer_result_t result, int idx,
                                           size_t* size);
    const char* slicer_last_error(void);
    void slicer_free_config(slicer_config_t config);
    void slicer_free_mesh(slicer_mesh_t mesh);
    void slicer_free_result(slicer_result_t result);

    - Add format, DPI, output-bounds, and normalization settings to config.
    - Define whether returned SVG strings and PNG bytes remain valid until
      result destruction; callers must copy them if needed longer.
    - Return status codes and thread-local/retrievable error text; null alone
      is not an adequate error contract.
    - All memory managed by engine (caller frees via *_free_*)
    - No C++ types in the header — pure C
    - Unit test: Call from C, verify success/failure diagnostics, lifetimes,
      invalid indices, and cleanup on every failure path.
