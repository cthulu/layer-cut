# Step 05 - Cricut Page SVG Writer

Priority: P0

## Goal

Write physically sized page SVGs containing independently selectable cut or guide
groups.

## Cut SVG

- Root width and height are in millimetres.
- Root viewBox is `0 0 page_width page_height`.
- Emit one group per layer.
- Use names such as `CUT | Layer 000`.
- Preserve even-odd fill behavior for holes.
- Preserve disconnected contours.
- Emit no decorative stroke for cut geometry.
- Include layer index and Z metadata.
- Count every emitted `<path>` toward the page budget.

## Guide SVG

- Use exactly the same page dimensions and viewBox as the cut SVG.
- Emit one group per valid interface.
- Use names such as `PEN GUIDE | Layer 001 over Layer 000 | Inset 1.0mm`.
- Place each guide in the previous layer's tile.
- Emit no guide for an interface that cannot be safely generated.
- Make guide groups visually distinct, but do not rely on color to set Cricut's operation.

## Naming

Use:

```text
page_001_layers_000-019.cut.svg
page_001_layers_000-019.guide.svg
```

Use SVG `id`, `data-layer-index`, `data-guide-for`, and `<title>` where possible.
Design Space behavior must be validated because standard SVG does not guarantee
that all metadata becomes a visible layer name.

## Acceptance

- XML parses successfully.
- Physical dimensions are correct.
- Groups and paths have deterministic order.
- Cut and guide files have identical page geometry.
- Holes and multiple contours remain correct.
- Path budgets are enforced before writing.
