#include "layer_cut.h"

#include <stddef.h>

typedef struct {
  int calls;
  int last_layer;
  int total_layers;
  double fraction;
} progress_state_t;

static void progress_callback(void* context, int current_layer, int total_layers,
                              double fraction) {
  progress_state_t* state = (progress_state_t*)context;
  state->calls += 1;
  state->last_layer = current_layer;
  state->total_layers = total_layers;
  state->fraction = fraction;
}

int main(void) {
  int status = 0;
  slicer_mesh_t mesh = slicer_load_stl(LAYER_CUT_SOURCE_DIR "/testfiles/fox.stl");
  if (!mesh) return 1;
  slicer_config_t config = slicer_config_create();
  if (!config || !slicer_config_set_layer_height(config, 0.2)) { status = 2; goto cleanup_mesh; }
  if (!slicer_config_set_cleanup_mode(config, 1) ||
      !slicer_config_set_cleanup_thresholds(config, 1.0, 0.0, 0.0, 0.0)) {
    status = 2; goto cleanup_config;
  }
  progress_state_t progress = {0, -1, 0, -1.0};
  if (!slicer_config_set_progress_callback(config, progress_callback, &progress)) {
    status = 2; goto cleanup_config;
  }
  slicer_result_t result = slicer_slice(mesh, config);
  if (!result || slicer_result_layer_count(result) != 533) { status = 3; goto cleanup_config; }
  if (progress.calls != 534 || progress.last_layer != 533 ||
      progress.total_layers != 533 || progress.fraction != 1.0) status = 3;
  const char* svg = slicer_result_layer_svg(result, 0);
  if (!svg || svg[0] == '\0' || slicer_result_layer_svg(result, -1) != NULL ||
      slicer_last_error()[0] == '\0') status = 4;
  slicer_free_result(result);
cleanup_config:
  slicer_free_config(config);
cleanup_mesh:
  slicer_free_mesh(mesh);
  return status;
}
