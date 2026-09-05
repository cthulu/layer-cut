STEP 6A — Manufacturing cleanup policy

Goal: Make small or fragile features explicit and controllable for cutting
machines without silently changing exact geometry.

Modes:
- preserve: return exact contours and do not emit cleanup warnings.
- warn: return exact contours and report features below configured limits.
- apply: explicitly modify geometry using Clipper2 offsets and area filtering.

Thresholds are measured in millimetres or square millimetres:
- minimum feature width
- minimum island area
- minimum hole width
- minimum bridge width

The default mode is warn. Applied cleanup uses polygon opening to remove
features narrower than the minimum width and closing to close narrow holes.
Warnings and whether geometry changed must be available to CLI and C clients.
Exporters must not claim that preserve/warn output is more manufacturable than
the source geometry. UI clients must show a pre-export summary before applying
changes.
