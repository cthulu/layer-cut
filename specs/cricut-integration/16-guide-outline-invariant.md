# Step 16 - Cricut Guide Outline Invariant

Priority: P0

## Goal

Ensure every Cricut pen guide retains an outline path, even when the configured
inset cannot be contained by the previous layer.

## Behavior

- Use the configured inward inset when its result is valid and contained.
- If the inset is invalid, collapses, or falls outside the previous layer,
  emit a warning and use the next layer's un-inset outline instead.
- Emit the fallback outline in both separate guide SVGs and combined SVGs.
- Preserve existing validation, path limits, transforms, and useful diagnostics.

## Acceptance Criteria

- A guide whose inset lies outside the previous layer still contains its guide
  group and outline path in both output forms.
- A normal contained inset continues to emit the inset geometry.
- Invalid inset and geometry conditions still produce warnings.
