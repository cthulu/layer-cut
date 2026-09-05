# STEP 19 - Export Workflow

Priority: P0

## Goal

Export the active profile to a user-selected directory.

## Supported Outputs

- One SVG per layer.
- One PNG per layer.
- Separate Cricut cut and guide SVGs per page.
- Combined Cricut SVG per page.
- Stacked STL preview.

Cricut PNG pages remain intentionally unimplemented.

## Requirements

- Use a native directory picker with security-scoped access.
- Show output profile, format, page/layer count, total size, and warnings.
- Use deterministic filenames from the engine/CLI contracts.
- Run exports off the main actor.
- Show progress, cancellation, partial-failure details, and completion state.
- Do not overwrite existing files without explicit confirmation.
- Copy engine-owned bytes before releasing the result handle.
