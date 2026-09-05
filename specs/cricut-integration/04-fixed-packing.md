# Step 04 - Fixed Cricut Packing

Priority: P0

## Goal

Pack sequential layers into fixed-size tiles without rotation or model scaling.

## Page Presets

```text
normal: page 304 x 304 mm, usable 290 x 290 mm
large:  page 304 x 608 mm, usable 290 x 594 mm
border: 7 mm on all sides
```

## Requirements

- Use row-major order: left-to-right, then top-to-bottom.
- Use ascending layer index order.
- Do not rotate layers.
- Use a common tile coordinate system for every layer.
- Base the fixed tile dimensions on the model's shared XY bounds.
- Add the configured gap to the tile pitch.
- Default gap is 3 mm.
- Accept finite negative gaps.
- Warn when negative gaps cause tile rectangles to overlap.
- Reject geometry outside the usable rectangle.
- Reject a layer whose shared model bounds exceed the usable area.
- Never scale or split an oversized layer.
- Split output into additional pages as needed.
- Enforce a 4,500-path safety budget per output SVG.

## Determinism

Document the placement formula, floating-point tolerance, rounding policy, and
page numbering. Identical input and options must produce byte-stable placement
and stable filenames.

## Future Strategy

Reserve `tight-packing` in the internal strategy model. Do not implement it here.
It will later pack actual layer rectangles, still without rotation initially.
