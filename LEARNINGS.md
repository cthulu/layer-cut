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
