# STEP 16 - Engine Integration

Priority: P0

## Goal

Provide a typed Swift service over the C ABI without blocking the main actor.

## Services

- `MeshService`: load metadata and request normalized/transformed mesh snapshots.
- `SlicingService`: slice using the active profile and collect diagnostics.
- `LayerPreviewService`: expose the cached complete SVG layer result for the
  selected layer and its next-layer guide, including warnings and cancellation.
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

## Layer Preview Boundary

- Layer preview is always SVG and must not depend on the selected export format.
- The C ABI request includes an explicit layer-preview-generation flag. Stacked
  preview and export pass it disabled; the opt-in layer-preview request enables
  it.
- The engine owns contour offsets, guide containment, fallback outlines, and
  guide warnings; Swift only renders the returned SVG.
- The generation request includes the active transform, layer height, cleanup
  settings, guide inset, and current profile identity; selection is applied to
  the cached complete result.
- The service supports cancellation and rejects stale results at the app state
  boundary. See `17-layer-preview.md` for the complete-result versus targeted
  generation decision and benchmark requirements.
