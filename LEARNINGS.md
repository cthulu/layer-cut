# Camera/Viewport Development Learnings

## 1. Camera Initialization and Coordinates
- The physical camera rotation is managed by a quaternion basis, not directly by simple Euler angles (Yaw/Pitch).
- The previously used values produced an unintended top-down view because the pitch was negative, which is misinterpreted by the rendering stack.
- The desired isometric/three-quarter view was achieved using a positive pitch value of approximately `1.1222` radians and Yaw `0.00` (or adjusted values in the future).

## 2. Debugging and Verification
- The transiently added `onCameraChange` callback proved invaluable for correlating the displayed Yaw/Pitch values with the actual visual output.
- Direct display of these internal `Yaw` and `Pitch` values is a crucial debugging tool for future camera view adjustments.

## 3. Implementation Status
- The viewport now correctly maintains the target orientation using the values `Yaw: 0.00`, `Pitch: 1.1222` for the desired view, and the camera reset function reliably uses these values to return to this state.

## 4. Layer naming
- Cricut compatibility learning: Cricut Design Space may ignore the current combined-SVG layer naming and display every layer with the same name (for
example, `page_000_layers_000-021`). This is likely not fixable through SVG naming alone; treat it as a known limitation

## 5. Float collapse in stacked preview
- Engine geometry is double, but `Triangle` vertices are float. Contour points
  closer together than the float epsilon at their coordinate magnitude
  (e.g. ~7.6e-6 mm near x=72) collapse to identical vertices after the cast,
  producing zero-area triangles that strict STL writers reject.
- `add_triangle` in `stacked_preview.cpp` drops these slivers at emission;
  the STL writers intentionally stay strict (tested contract).

## 6. Sanitizer builds and Clipper2
- ASan reports a `container-overflow` inside Clipper2's `ClipperOffset::DoRound`
  (libc++ annotated vector interaction) on the manufacturing-cleanup offset path.
  It is pre-existing and third-party — do not chase it as a regression; compare
  against the base commit before investigating sanitizer failures.

## 7. CLI is a thin C ABI client
- `cli/main.cpp` uses only the public C API (`layer_cut.h`), never engine
  internals, so CLI and macOS app share one pipeline. Keep it that way.
- `--stacked-stl` writes the engine's binary STL (same bytes as the app's
  `stacked_preview.stl` export); the ASCII writer remains engine-internal.
