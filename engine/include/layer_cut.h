#ifndef LAYER_CUT_H
#define LAYER_CUT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* slicer_mesh_t;
typedef void* slicer_result_t;
typedef void* slicer_config_t;
typedef void* slicer_snapshot_t;
typedef void* slicer_cancellation_t;

typedef void (*slicer_progress_callback_t)(void* context,
                                           int current_layer,
                                           int total_layers,
                                            double fraction);

typedef struct slicer_bounds {
  float min_x, min_y, min_z;
  float max_x, max_y, max_z;
} slicer_bounds_t;

enum slicer_format {
  SLICER_FORMAT_SVG = 0,
  SLICER_FORMAT_PNG = 1,
  SLICER_FORMAT_CRICUT_NORMAL = 2,
  SLICER_FORMAT_CRICUT_LARGE = 3
};

slicer_mesh_t slicer_load_stl(const char* path);
slicer_config_t slicer_config_create(void);
int slicer_config_set_layer_height(slicer_config_t config, double mm);
int slicer_config_set_canvas(slicer_config_t config, double width_mm,
                             double height_mm);
int slicer_config_set_format(slicer_config_t config, int format);
int slicer_config_set_dpi(slicer_config_t config, int dpi);
int slicer_config_set_cricut_gap(slicer_config_t config, double gap_mm);
int slicer_config_set_cricut_packing(slicer_config_t config, int tight);
int slicer_config_set_cricut_guide_inset(slicer_config_t config, double inset_mm);
int slicer_config_set_show_layer_numbers(slicer_config_t config, int enabled);
int slicer_config_set_layer_number_font_size(slicer_config_t config, double size_mm);
int slicer_config_set_cleanup_mode(slicer_config_t config, int mode);
int slicer_config_set_progress_callback(slicer_config_t config,
                                         slicer_progress_callback_t callback,
                                         void* context);
int slicer_config_set_transform(slicer_config_t config, int axis,
                                double rotation_degrees, double scale);
int slicer_config_set_transform_euler(slicer_config_t config, int axis,
                                      double rotate_x_degrees,
                                      double rotate_y_degrees,
                                      double rotate_z_degrees, double scale);
int slicer_config_set_cancellation(slicer_config_t config,
                                   slicer_cancellation_t cancellation);
int slicer_config_set_cleanup_thresholds(slicer_config_t config,
                                         double feature_width_mm,
                                         double island_area_mm2,
                                         double hole_width_mm,
                                         double bridge_width_mm);
slicer_result_t slicer_slice(slicer_mesh_t mesh, slicer_config_t config);
int slicer_result_layer_count(slicer_result_t result);
double slicer_result_layer_z(slicer_result_t result, int index);
int slicer_result_layer_is_empty(slicer_result_t result, int index);
const uint8_t* slicer_result_layer_png(slicer_result_t result, int index,
                                       size_t* size);
const char* slicer_result_layer_svg(slicer_result_t result, int index);
size_t slicer_result_layer_svg_size(slicer_result_t result, int index);
int slicer_result_page_count(slicer_result_t result);
const char* slicer_result_page_cut_svg(slicer_result_t result, int page);
size_t slicer_result_page_cut_svg_size(slicer_result_t result, int page);
const char* slicer_result_page_guide_svg(slicer_result_t result, int page);
size_t slicer_result_page_guide_svg_size(slicer_result_t result, int page);
const char* slicer_result_page_combined_svg(slicer_result_t result, int page);
size_t slicer_result_page_combined_svg_size(slicer_result_t result, int page);
int slicer_result_page_layer_start(slicer_result_t result, int page);
int slicer_result_page_layer_count(slicer_result_t result, int page);
int slicer_result_page_path_count(slicer_result_t result, int page);
int slicer_result_warning_count(slicer_result_t result);
const char* slicer_result_warning(slicer_result_t result, int index);
size_t slicer_result_warning_size(slicer_result_t result, int index);
const uint8_t* slicer_result_stacked_stl(slicer_result_t result, size_t* size);
int slicer_result_diagnostic_count(slicer_result_t result);
const char* slicer_result_diagnostic(slicer_result_t result, int index);
int slicer_result_diagnostic_severity(slicer_result_t result, int index);

int slicer_mesh_triangle_count(slicer_mesh_t mesh);
int slicer_mesh_bounds(slicer_mesh_t mesh, slicer_bounds_t* bounds);
double slicer_mesh_volume(slicer_mesh_t mesh);
int slicer_mesh_diagnostic_count(slicer_mesh_t mesh);
const char* slicer_mesh_diagnostic(slicer_mesh_t mesh, int index);
int slicer_mesh_diagnostic_severity(slicer_mesh_t mesh, int index);
int slicer_mesh_snapshot(slicer_mesh_t mesh, int axis, double rotation_degrees,
                         double scale, slicer_snapshot_t* snapshot);
int slicer_mesh_snapshot_euler(slicer_mesh_t mesh, int axis,
                               double rotate_x_degrees,
                               double rotate_y_degrees,
                               double rotate_z_degrees, double scale,
                               slicer_snapshot_t* snapshot);
const float* slicer_snapshot_vertices(slicer_snapshot_t snapshot, size_t* count);
const uint32_t* slicer_snapshot_indices(slicer_snapshot_t snapshot, size_t* count);
int slicer_snapshot_bounds(slicer_snapshot_t snapshot, slicer_bounds_t* bounds);

slicer_cancellation_t slicer_cancellation_create(void);
void slicer_cancellation_cancel(slicer_cancellation_t cancellation);
int slicer_cancellation_is_cancelled(slicer_cancellation_t cancellation);
void slicer_free_cancellation(slicer_cancellation_t cancellation);
void slicer_free_snapshot(slicer_snapshot_t snapshot);
const char* slicer_last_error(void);
void slicer_free_config(slicer_config_t config);
void slicer_free_mesh(slicer_mesh_t mesh);
void slicer_free_result(slicer_result_t result);

#ifdef __cplusplus
}
#endif

#endif
