# Cricut Integration Plan

## Scope

Add Cricut Design Space-oriented page exports and an accurate stacked STL preview.
Execute these specs in numeric order unless a later spec is explicitly marked as
independent.

## Priorities

- P0: Required to validate and ship the first Cricut workflow.
- P1: Required for the stacked STL preview and production-quality guides.
- P2: Required for Swift/C ABI consumers after the CLI path is stable.
- P3: Future optimization, not required for the first release.

## Decisions

- Add `cricut-normal` for a 304 x 304 mm page.
- Add `cricut-large` for a 304 x 608 mm page.
- Reserve 7 mm on every side; usable areas are 290 x 290 mm and 290 x 594 mm.
- Generate multiple pages when all layers do not fit on one page.
- Use fixed row-major packing first; fixed packing never rotates layers.
- Tight packing may use deterministic orthogonal layer rotations (`0`, `90`,
  `180`, `270` degrees).
- Default the inter-tile gap to 3 mm and allow negative gaps with warnings.
- Reject an oversized layer instead of scaling or splitting it.
- Enforce a conservative 4,500-path limit per SVG page.
- Generate separate `.cut.svg` and `.guide.svg` files for every page.
- Support an optional combined SVG page form with grouped cut and pen layers and
  no alignment marker.
- Cricut PNG pages are explicitly deferred and are not implemented by this plan.
- Use one guide file per page, with one independently selectable guide group per interface.
- Use true inward offsets in millimetres for production guides.
- Omit guides that cannot be contained by the previous layer and emit warnings.
- `tight-packing` is implemented as an optional deterministic shelf strategy;
  `fixed` remains the default.
- Keep layer numbering disabled by default and limited to combined Cricut SVGs.
- Add an optional accurate, watertight stacked STL preview.
- Do not attempt automated Design Space validation; validation is manual only.

## Rejected Alternatives

- Registration holes or sacrificial tabs: extra material is discarded.
- Visual-only alignment: errors accumulate during gluing.
- Combined cut and guide SVGs: separate files make Pen conversion easier.
- Uniform 95% guide scaling: it does not guarantee containment.
- Automatic Cut/Pen assignment in SVG: Design Space imports SVG vectors as Cut by default.

## Execution Order

1. Five-slice Design Space validation spike.
2. Standalone ASCII STL writer.
3. Shared page model.
4. Fixed packing.
5. Page SVG writer.
6. Cricut CLI integration.
7. Cricut automated tests.
8. Production guide offsets.
9. Polygon triangulation.
10. Stacked preview mesh.
11. Stacked STL CLI integration and tests.
12. Stacked STL tests.
13. C ABI page access.
14. Tight packing and orthogonal rotation.
15. Design Space validation record.
16. Guide outline invariant.
17. Combined SVG layer numbering.

`15-design-space-validation.md` is a cross-cutting validation record and should
be completed immediately after the sample spike, before production assumptions
are finalized, despite its filename.
