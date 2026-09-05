# STEP 08A — PNG generation progress reporting

## Goal

Expose progress while PNG layers are being generated so the UI can show the
current layer, total layer count, and percentage completion.

## Scope

- Report progress once per completed layer, after rasterization and PNG
  encoding have succeeded.
- Progress is reported for the complete output operation, not for individual
  pixels or supersampling samples.
- Preserve deterministic output and existing PNG dimensions, fill rule,
  antialiasing, and DPI metadata.

## Progress contract

Define a C ABI progress callback before exposing it to Swift:

```c
typedef void (*slicer_progress_callback_t)(
    void* context,
    int current_layer,
    int total_layers,
    double fraction);
```

- `current_layer` is one-based for completed layers; it is `0` before the
  first layer begins.
- `total_layers` is the final number of layers and remains constant.
- `fraction` is clamped to `[0.0, 1.0]` and equals
  `current_layer / total_layers`; an empty result reports `1.0`.
- A null callback disables progress reporting.
- The callback must not own, retain, or mutate engine memory.
- Callbacks execute on the worker thread performing the slice. The UI must
  dispatch updates to the main actor/main thread.
- The callback must be optional and must not change existing callers that do
  not request progress.

## Configuration and lifetime

- Add callback and context fields to the slicing configuration.
- The caller owns the callback context for the entire `slicer_slice` call.
- Do not retain the callback or context after `slicer_slice` returns.
- Document that cancellation is not part of this substep; cancellation must
  be designed separately before the UI exposes a cancel action.

## UI requirements

The progress view displays:

- `Layer <current> of <total>`
- A determinate progress bar whose value is `fraction`
- A percentage rounded to the nearest whole number
- A completed state after the final layer is encoded
- An error state that stops progress and shows the engine diagnostic

The UI must remain responsive by running slicing off the main actor/thread.
Progress updates must not cause one UI render per pixel or per scanline.

## Acceptance tests

- A multi-layer PNG slice reports an initial `0 / N` state and exactly one
  completion update for each layer through `N / N`.
- The final fraction is exactly `1.0` and the displayed percentage is `100%`.
- A one-layer slice reports `1 / 1` and `100%` after encoding.
- An empty-layer result reports completion without division by zero.
- A null callback leaves existing slicing behavior unchanged.
- A failed layer does not report it as completed and preserves the existing
  error result.
- Callback values are monotonic, bounded, and correct when PNG generation is
  slower than SVG generation.
- Existing PNG byte, dimension, DPI metadata, and output tests continue to
  pass unchanged.
