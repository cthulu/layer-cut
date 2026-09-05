# Step 06 - Cricut CLI Output

Priority: P0

## Goal

Expose Cricut page generation through the existing CLI without changing SVG or
PNG behavior.

## Format Values

```text
--format cricut-normal
--format cricut-large
```

Keep existing `svg` and `png` values unchanged.

## Options

Add options equivalent to:

```text
--packing fixed
--cricut-gap 3
--cricut-guide-inset 1
```

The first implementation supports only `fixed`. Reserve `tight-packing` and
return a clear unsupported-strategy error until its spec is implemented.

## Behavior

- Create the output directory as the existing CLI does.
- Generate both cut and guide files for every page.
- Report page count, layer ranges, path counts, and warnings.
- Report omitted guides and their reasons.
- Use current exit-code conventions: 0 success, 1 runtime error, 2 invalid input.
- Reuse existing STL loading, normalization, slicing, union, and cleanup policies.

## Help Text

Document page dimensions, 7 mm borders, millimetre units, fixed packing, gap
semantics, negative-gap warnings, guide behavior, and oversized-layer rejection.
