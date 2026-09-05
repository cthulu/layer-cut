# STEP 16 - Engine Integration

Priority: P0

## Goal

Provide a typed Swift service over the C ABI without blocking the main actor.

## Services

- `MeshService`: load metadata and request normalized/transformed mesh snapshots.
- `SlicingService`: slice using the active profile and collect diagnostics.
- `ExportService`: collect per-layer outputs, Cricut pages, combined pages, and
  stacked STL output.
- `ProfileStore`: load/save validated YAML profiles.

## C ABI Requirements

Extend the ABI for transform configuration before slicing, mesh snapshot
vertices/indices/bounds/lifetime, Cricut page access, progress, cancellation,
and diagnostics.

## Concurrency and Lifetime

- Run load, transform, slice, and export work in a Swift task or worker.
- Check cancellation between expensive engine phases.
- Convert C diagnostics into typed Swift errors and warnings.
- Release every mesh, config, result, and snapshot handle in `defer` blocks.
- Copy bytes/data at the service boundary before releasing C handles.
