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
  slicer_snapshot_t snapshot = NULL;
  size_t snapshot_vertices = 0;
  if (!slicer_mesh_snapshot(mesh, 4, 0.0, 1.0, &snapshot) || !snapshot ||
      !slicer_snapshot_vertices(snapshot, &snapshot_vertices) ||
      snapshot_vertices == 0) {
    status = 1;
  }
  slicer_free_snapshot(snapshot);
  slicer_config_t config = slicer_config_create();
  if (!config || !slicer_config_set_layer_height(config, 0.2)) { status = 2; goto cleanup_mesh; }
  if (!slicer_config_set_cleanup_mode(config, 1) ||
      !slicer_config_set_cleanup_thresholds(config, 1.0, 0.0, 0.0, 0.0)) {
    status = 2; goto cleanup_config;
  }
  if (!slicer_config_set_transform(config, 4, 0.0, 1.0)) {
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

  slicer_config_t page_config = slicer_config_create();
  if (!page_config || !slicer_config_set_layer_height(page_config, 0.2) ||
      !slicer_config_set_format(page_config, SLICER_FORMAT_CRICUT_NORMAL) ||
      !slicer_config_set_cricut_gap(page_config, 3.0) ||
      !slicer_config_set_cricut_guide_inset(page_config, 1.0)) {
    status = 5;
    slicer_free_config(page_config);
    goto cleanup_config;
  }
  slicer_result_t pages = slicer_slice(mesh, page_config);
  if (!pages || slicer_result_page_count(pages) != 45 ||
      slicer_result_page_layer_start(pages, 0) != 0 ||
      slicer_result_page_layer_count(pages, 0) != 12 ||
      slicer_result_page_path_count(pages, 0) <= 0) {
    status = 6;
  }
  const char* cut_page = slicer_result_page_cut_svg(pages, 0);
  const char* guide_page = slicer_result_page_guide_svg(pages, 0);
  if (!cut_page || !guide_page ||
      slicer_result_page_cut_svg_size(pages, 0) == 0 ||
      slicer_result_page_guide_svg_size(pages, 0) == 0 ||
      slicer_result_page_cut_svg(pages, -1) != NULL ||
      slicer_last_error()[0] == '\0') {
      status = 7;
  }
  size_t stacked_size = 0;
  if (!slicer_result_stacked_stl(pages, &stacked_size) || stacked_size == 0 ||
      !slicer_result_page_combined_svg(pages, 0)) status = 7;
  slicer_free_result(pages);
  slicer_free_config(page_config);
  slicer_cancellation_t cancellation = slicer_cancellation_create();
  slicer_config_t cancelled_config = slicer_config_create();
  if (!cancellation || !cancelled_config ||
      !slicer_config_set_cancellation(cancelled_config, cancellation)) {
    status = 8;
  } else {
    slicer_cancellation_cancel(cancellation);
    if (slicer_slice(mesh, cancelled_config) != NULL) status = 8;
  }
  slicer_free_config(cancelled_config);
  slicer_free_cancellation(cancellation);
cleanup_config:
  slicer_free_config(config);
cleanup_mesh:
  slicer_free_mesh(mesh);
  return status;
}
