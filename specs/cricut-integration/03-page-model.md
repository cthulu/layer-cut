# Step 03 - Cricut Page Model

Priority: P0

## Goal

Introduce one shared internal representation for cut pages and guide pages so
that placement cannot diverge between the two output files.

## Model

Define page data for:

- Physical page width and height.
- Usable rectangle after borders.
- Page index and inclusive layer range.
- Fixed tile width, height, and gap.
- Tile row and column.
- Layer index and Z position.
- Transformed cut contours.
- Transformed guide contours.
- Per-page path count.
- Warnings and omitted guides.

Use a strategy abstraction with `fixed` implemented first and `tight-packing`
reserved for a later step.

## Geometry Contract

- All layers retain one shared model XY coordinate system.
- Do not independently center or normalize layers to their own bounds.
- Cut and guide output must use the same page transforms.
- A guide named `Layer N+1 over Layer N` is placed in Layer N's tile.
- Page coordinates use millimetres and a page viewBox origin of `(0, 0)`.
- Layer Z metadata remains the original midpoint Z.

## Acceptance

- The model can represent a page containing cut groups and guide groups.
- Both SVG writers consume the same transformed geometry.
- Page and tile coordinates are deterministic.
- No output-specific code recomputes placement independently.
