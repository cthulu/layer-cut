# Step 13 - Cricut Page C ABI

Priority: P2

## Goal

Expose Cricut pages to the future SwiftUI application without breaking the
existing one-layer C ABI.

## API Direction

Add page-oriented accessors rather than overloading layer accessors. Candidate
functions include:

```c
int slicer_result_page_count(slicer_result_t result);
const char* slicer_result_page_cut_svg(slicer_result_t result, int page);
size_t slicer_result_page_cut_svg_size(slicer_result_t result, int page);
const char* slicer_result_page_guide_svg(slicer_result_t result, int page);
size_t slicer_result_page_guide_svg_size(slicer_result_t result, int page);
int slicer_result_page_layer_start(slicer_result_t result, int page);
int slicer_result_page_layer_count(slicer_result_t result, int page);
int slicer_result_page_path_count(slicer_result_t result, int page);
```

Names may be adjusted to the existing ABI style, but the contract must define
index validation, returned-byte lifetime, warnings, page dimensions, and layer
ranges.

## Requirements

- Keep the header pure C.
- Preserve existing per-layer APIs.
- Keep result-owned memory valid until `slicer_free_result`.
- Add C tests for valid and invalid page access.
- Expose omitted-guide warnings and path-budget failures.
