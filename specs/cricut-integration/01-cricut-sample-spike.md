# Step 01 - Five-Slice Cricut Validation Spike

Priority: P0

## Goal

Create a small, disposable generator that validates Design Space behavior before
the production page exporter is implemented.

## Inputs

Use these existing files:

```text
output/layer_000.svg
output/layer_001.svg
output/layer_002.svg
output/layer_003.svg
output/layer_004.svg
```

The current fixture has 213 layers and 231 total paths: 195 layers have one
path and 18 layers have two paths.

## Outputs

```text
page_001_layers_000-004.cut.svg
page_001_layers_000-004.guide.svg
```

## Requirements

- Use a 304 x 304 mm page with a 7 mm border.
- Use deterministic fixed packing with five layers in a 3-column by 2-row grid.
- Preserve physical dimensions and the existing model coordinates.
- Put each cut layer in a named group such as `CUT | Layer 000`.
- Put each prototype guide in a named group such as `PEN GUIDE | Layer 001 over Layer 000 | Prototype`.
- Put a guide in the previous layer's tile, not the next layer's tile.
- Use distinct presentation colors for cut and guide groups.
- Include useful SVG `id`, `data-*`, and `<title>` metadata.
- Use a provisional 5% scaled guide only for this spike; production uses true offsets.
- Do not make this generator a runtime dependency of the engine.

## Validation

Manually verify in Cricut Design Space that:

- Both files upload successfully.
- Physical page and layer dimensions are preserved.
- Groups are visible and independently selectable.
- Guide groups can be changed to `Draw > Pen`.
- A guide group can be attached to its corresponding cut layer.
- The user can plot one selected guide on the previous slice.
- The page remains below the 5,000-path limit.

## Deliverable

Provide the generator, invocation instructions, generated sample files, and a
short record of Design Space behavior. This step may be implemented as a script
under `tools/` or as a temporary test executable.
