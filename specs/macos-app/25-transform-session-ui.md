# STEP 25 - Session Transform Controls

Priority: P0

## State Ownership

Cutting axis, X/Y/Z rotation, and scale are session-only macOS app state. They
must not be stored in `SlicingProfile` or profile YAML.

Profiles continue to own slicing, cleanup, packing, and output settings. The
active session transform is combined with the active profile when calling the
engine.

## Parameters UI

Replace the current orientation controls with:

- `Cutting axis` picker for `+X`, `-X`, `+Y`, `-Y`, `+Z`, and `-Z`.
- `Rotate X (deg)` slider.
- `Rotate Y (deg)` slider.
- `Rotate Z (deg)` slider.
- `Scale` control.

All controls use the engine's validation ranges and numeric formatting.

## Viewport

- The transformed mesh snapshot must use the session transform.
- Reset transform restores `+Z`, zero rotations, and scale `1`.
- Camera reset remains independent from transform reset.
- Camera orbit must not modify session transform values.
