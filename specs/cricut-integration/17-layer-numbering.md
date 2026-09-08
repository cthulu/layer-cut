# Step 17 - Combined SVG Layer Numbering

Priority: P1

## Goal

Optionally label each generated layer in the combined Cricut SVG so assembled
layers can be identified without changing cut or guide geometry.

## Scope

- Numbering is available only for combined Cricut SVG output.
- Normal per-layer SVG, PNG, separate cut SVG, and separate guide SVG outputs are
  unchanged.
- Numbering is disabled by default.
- The engine is solely responsible for label content, placement, formatting, and
  SVG generation. UI clients must not calculate label positions.

## Configuration

Add a boolean engine configuration value, `show_layer_numbers`, defaulting to
`false`. Expose it through the C ABI, the CLI as an explicit opt-in flag such as
`--layer-numbers`, and the macOS profile model and versioned persistence.

The setting is effective only for combined Cricut output. It may remain stored in
profiles when another output is selected, but must not add numbering to other
formats.

## SVG Output

When enabled, each combined page must contain one additional root-level group,
separate from `cut-layers` and `pen-layers`:

```xml
<g id="layer-numbers" data-operation="layer-number">
  ...
</g>
```

The group must be easy to disable as a unit in SVG editors and must contain one
label per non-empty layer present on that page. Each label must include stable
metadata such as `data-layer-index`, an informative `id`, and a `<title>`.

Use SVG text with:

- `font-size="5mm"` exactly.
- A standard generic SVG font family, not a platform-specific dependency.
- Deterministic C-locale-compatible numeric serialization.
- Centered horizontal and vertical alignment around the geometric center of the
  layer's actual emitted slice bounds.

The displayed number is one-based while the zero-based layer index remains in
metadata. Labels follow the layer's final packed translation and orthogonal
rotation. Empty layers have no label.

Numbering is annotation only. It must not be placed inside `cut-layers` or
`pen-layers`, must not receive cut/pen operation metadata, and must not count as a
cut or guide path toward the 4,500-path budget.

## Tests

- Verify numbering is absent by default.
- Verify the option affects only combined Cricut SVG output.
- Verify one label is emitted for every non-empty page layer and none for empty
  layers.
- Verify the label group is separate and can be disabled without changing cut or
  guide groups.
- Verify 5 mm font size, metadata, one-based display number, and deterministic
  C-locale serialization.
- Verify centers for negative bounds, holes, disconnected contours, and rotated
  tightly packed layers.
- Verify repeated exports are byte-stable.
