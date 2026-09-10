# STEP 26 - Transform Reset and Output Consistency

Priority: P0

## Model Changes

When a new STL is opened, reset the session transform to:

```text
+Z, rotate X/Y/Z = 0, scale = 1
```

Do not reset the transform when changing only profile slicing or output settings.

## Preview and Export

The current session transform must be passed identically to:

- RealityKit mesh snapshots.
- Layer SVG and PNG generation.
- Cricut page generation.
- Stacked STL generation.
- Any stacked-STL reload used for the 3D preview.

Preview cache identities must include the model path and every transform value.
Changing any transform invalidates transformed mesh and layer previews.

No transform value may be persisted in profile YAML, export metadata, or shared
global settings.
