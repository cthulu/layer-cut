# STEP 14 - File Import UI

Priority: P0

## Goal

Load an STL through a native macOS file dialog or drag-and-drop flow.

## Requirements

- Use SwiftUI `fileImporter` with an STL content type and drag-and-drop support.
- Pass the selected URL to the C++ loader; never parse STL in Swift.
- Display filename, file size, triangle count, raw bounds, and normalized bounds.
- Display the millimetre assumption; never infer units from STL metadata.
- Surface invalid, truncated, non-finite, oversized, and unsupported ASCII STL
  diagnostics from the engine.
- Keep security-scoped URL access alive only for the operation that needs it.
- Make the imported document the source for the active profile and viewport.
