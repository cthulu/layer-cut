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
- Emit the shared alignment marker/registration geometry using the page model's
  exact coordinates.
- Mark the cut-file marker with `data-alignment-marker-operation="ignore"`; the
  user must leave it out of the Cut operation in Design Space.

## Guide SVG

- Use exactly the same page dimensions and viewBox as the cut SVG.
- Emit one group per valid interface.
- Use names such as `PEN GUIDE | Layer 001 over Layer 000 | Inset 1.0mm`.
- Place each guide in the previous layer's tile.
- Emit no guide for an interface that cannot be safely generated.
- Make guide groups visually distinct, but do not rely on color to set Cricut's operation.
- Emit the exact same alignment marker geometry at the exact same coordinates as
  the paired cut SVG. The marker exists to align the two imported documents in
  Design Space, not as a cut or pen operation.
- Mark the guide-file marker with `data-alignment-marker-operation="draw"`; the
  marker is intended for the Pen operation.

## Combined SVG Mode

Support an optional single-file page form. It must:

- Emit one `page_*.svg` file per page.
- Contain a `cut-layers` group with `data-operation="cut"`.
- Contain a `pen-layers` group with `data-operation="draw"`.
- Include all cut and valid pen-guide groups in their respective parent group.
- Omit alignment-marker geometry entirely.
- Preserve the same page dimensions, tile coordinates, and guide inset as the
  separate-file form.

## Naming

Use:

```text
page_001_layers_000-019.cut.svg
page_001_layers_000-019.guide.svg
```

Use SVG `id`, `data-layer-index`, `data-guide-for`, and `<title>` where possible.
Design Space currently imports these groups as generic `Group` entries rather
than displaying the SVG names. Treat metadata as diagnostic only; rely on stable
ordering, visual styling, page filenames, and the shared alignment marker for
user guidance.

## Acceptance

- XML parses successfully.
- Physical dimensions are correct.
- Groups and paths have deterministic order.
- Cut and guide files have identical page geometry.
- Paired cut and guide files contain identical alignment marker geometry and
  coordinates.
- Holes and multiple contours remain correct.
- Path budgets are enforced before writing.
