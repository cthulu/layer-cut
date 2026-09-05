# Step 11 - Stacked STL CLI Option

Priority: P1

## Goal

Expose stacked preview generation as an optional CLI output.

## Interface

Add:

```text
--stacked-stl preview.stl
```

The option may be used with normal SVG, PNG, or Cricut output. It must not change
the existing output unless explicitly supplied.

## Behavior

- Use the configured layer height.
- Use the same normalization, union, and cleanup options as the selected output.
- Generate all scheduled layers by default.
- Report the output path and triangle count.
- Report triangulation or STL serialization failures clearly.
- Return existing CLI exit codes.
- Document that the STL is a stacked layer visualization, not a replacement for
  the original mesh.

## Help Text

Explain that the preview uses physical layer thickness and millimetre units and
is intended to verify the approximate assembled design.
