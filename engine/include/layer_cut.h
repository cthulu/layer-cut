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

typedef void (*slicer_progress_callback_t)(void* context,
                                           int current_layer,
                                           int total_layers,
                                           double fraction);

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
int slicer_config_set_cricut_guide_inset(slicer_config_t config, double inset_mm);
int slicer_config_set_cleanup_mode(slicer_config_t config, int mode);
int slicer_config_set_progress_callback(slicer_config_t config,
                                        slicer_progress_callback_t callback,
                                        void* context);
int slicer_config_set_cleanup_thresholds(slicer_config_t config,
                                         double feature_width_mm,
                                         double island_area_mm2,
                                         double hole_width_mm,
                                         double bridge_width_mm);
slicer_result_t slicer_slice(slicer_mesh_t mesh, slicer_config_t config);
int slicer_result_layer_count(slicer_result_t result);
const uint8_t* slicer_result_layer_png(slicer_result_t result, int index,
                                       size_t* size);
const char* slicer_result_layer_svg(slicer_result_t result, int index);
size_t slicer_result_layer_svg_size(slicer_result_t result, int index);
int slicer_result_page_count(slicer_result_t result);
const char* slicer_result_page_cut_svg(slicer_result_t result, int page);
size_t slicer_result_page_cut_svg_size(slicer_result_t result, int page);
const char* slicer_result_page_guide_svg(slicer_result_t result, int page);
size_t slicer_result_page_guide_svg_size(slicer_result_t result, int page);
int slicer_result_page_layer_start(slicer_result_t result, int page);
int slicer_result_page_layer_count(slicer_result_t result, int page);
int slicer_result_page_path_count(slicer_result_t result, int page);
int slicer_result_warning_count(slicer_result_t result);
const char* slicer_result_warning(slicer_result_t result, int index);
size_t slicer_result_warning_size(slicer_result_t result, int index);
const char* slicer_last_error(void);
void slicer_free_config(slicer_config_t config);
void slicer_free_mesh(slicer_mesh_t mesh);
void slicer_free_result(slicer_result_t result);

#ifdef __cplusplus
}
#endif

#endif
