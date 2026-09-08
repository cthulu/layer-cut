# STEP 15 - Profile and Parameters UI

Priority: P0

## Goal

Provide a profile-driven UI for transforms, slicing, cleanup, and output formats.

## Default Profile

Create a built-in `Default` profile with:

- Layer height: 1.0 mm.
- Identity transform.
- Output: `cricut-normal`.
- Fixed packing.
- Cricut gap: 3 mm.
- Guide inset: 1 mm.
- Cleanup mode: warn.

## UI Groups

- Model orientation: axis preset, fine rotation, scale.
- Slicing: layer height and estimated layer count.
- Bounds: normalized model bounds read-only; output canvas separate.
- Cleanup: preserve, warn, apply, and thresholds.
- Output: SVG, PNG, Cricut normal, Cricut large, combined Cricut SVG, stacked STL.
- Combined Cricut SVG: optional layer numbers, disabled by default.
- Cricut: packing, gap, guide inset, and page estimate.

## Profiles

- Allow creating, renaming, duplicating, editing, and deleting user profiles.
- Store profiles as versioned YAML.
- Load from `$XDG_CONFIG_HOME/layer-cut` or the macOS Application Support fallback.
- Validate on load and recover from invalid files using the built-in default.
- Write atomically through a temporary file and rename.
- Do not silently change a profile when an output format gains new options.

## UI Guardrails

- The panel passes configuration to the engine and displays engine-provided
  results; it does not calculate geometry, packing, or label positions.
- Numeric controls use C-format values with `.` as the decimal separator.
- Slider controls use the standard two-row label/value and slider layout.
- Layer numbering is an opt-in toggle and is only effective for combined Cricut
  SVG output.
