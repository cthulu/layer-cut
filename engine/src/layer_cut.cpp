#include "layer_cut.h"

#include "mesh.h"
#include "polygon_ops.h"
#include "png_writer.h"
#include "slicer.h"
#include "stl_loader.h"
#include "svg_writer.h"

#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {
thread_local std::string last_error;

struct MeshHandle { layer_cut::Mesh mesh; };
struct ConfigHandle {
  double layer_height = 0.2;
  double canvas_width = 0.0;
  double canvas_height = 0.0;
  int format = SLICER_FORMAT_SVG;
  int dpi = 300;
  int cleanup_mode = 1;
  layer_cut::ManufacturingCleanupOptions cleanup;
};
struct LayerBytes {
  std::string svg;
  std::vector<uint8_t> png;
};
struct ResultHandle {
  std::vector<LayerBytes> layers;
  std::vector<std::string> warnings;
};

template <typename T> T* handle(void* value) { return static_cast<T*>(value); }
void fail(std::string message) { last_error = std::move(message); }
bool valid_handle(const void* value) { return value != nullptr; }
}

extern "C" {

slicer_mesh_t slicer_load_stl(const char* path) {
  last_error.clear();
  if (path == nullptr) { fail("STL path is null"); return nullptr; }
  layer_cut::LoadError error;
  const auto triangles = layer_cut::load_stl(path, {}, &error);
  if (error.code != layer_cut::LoadError::Code::OK) { fail(error.message); return nullptr; }
  const auto validation = layer_cut::validate_and_normalize(triangles);
  if (validation.has_errors()) { fail("STL mesh validation failed"); return nullptr; }
  auto* result = new MeshHandle;
  result->mesh = validation.mesh;
  return result;
}

slicer_config_t slicer_config_create(void) { return new ConfigHandle; }

int slicer_config_set_layer_height(slicer_config_t config, double mm) {
  if (!valid_handle(config) || !std::isfinite(mm) || mm <= 0.0) {
    fail("Layer height must be positive and finite"); return 0;
  }
  handle<ConfigHandle>(config)->layer_height = mm; return 1;
}

int slicer_config_set_canvas(slicer_config_t config, double width_mm, double height_mm) {
  if (!valid_handle(config) || !std::isfinite(width_mm) || !std::isfinite(height_mm) ||
      width_mm <= 0.0 || height_mm <= 0.0) {
    fail("Canvas dimensions must be positive and finite"); return 0;
  }
  handle<ConfigHandle>(config)->canvas_width = width_mm;
  handle<ConfigHandle>(config)->canvas_height = height_mm; return 1;
}

int slicer_config_set_format(slicer_config_t config, int format) {
  if (!valid_handle(config) || (format != SLICER_FORMAT_SVG && format != SLICER_FORMAT_PNG)) {
    fail("Unsupported output format"); return 0;
  }
  handle<ConfigHandle>(config)->format = format; return 1;
}

int slicer_config_set_dpi(slicer_config_t config, int dpi) {
  if (!valid_handle(config) || dpi <= 0) { fail("DPI must be positive"); return 0; }
  handle<ConfigHandle>(config)->dpi = dpi; return 1;
}

int slicer_config_set_cleanup_mode(slicer_config_t config, int mode) {
  if (!valid_handle(config) || mode < 0 || mode > 2) {
    fail("Cleanup mode must be 0 (preserve), 1 (warn), or 2 (apply)"); return 0;
  }
  handle<ConfigHandle>(config)->cleanup_mode = mode; return 1;
}

int slicer_config_set_cleanup_thresholds(slicer_config_t config, double feature_width_mm,
                                         double island_area_mm2, double hole_width_mm,
                                         double bridge_width_mm) {
  if (!valid_handle(config) || !std::isfinite(feature_width_mm) ||
      !std::isfinite(island_area_mm2) || !std::isfinite(hole_width_mm) ||
      !std::isfinite(bridge_width_mm) || feature_width_mm < 0.0 ||
      island_area_mm2 < 0.0 || hole_width_mm < 0.0 || bridge_width_mm < 0.0) {
    fail("Cleanup thresholds must be finite and non-negative"); return 0;
  }
  auto& cleanup = handle<ConfigHandle>(config)->cleanup;
  cleanup.minimum_feature_width = feature_width_mm;
  cleanup.minimum_island_area = island_area_mm2;
  cleanup.minimum_hole_width = hole_width_mm;
  cleanup.minimum_bridge_width = bridge_width_mm;
  return 1;
}

slicer_result_t slicer_slice(slicer_mesh_t mesh, slicer_config_t config) {
  last_error.clear();
  if (!valid_handle(mesh) || !valid_handle(config)) { fail("Mesh and config are required"); return nullptr; }
  auto* mesh_handle = handle<MeshHandle>(mesh);
  auto* config_handle = handle<ConfigHandle>(config);
  const auto sliced = layer_cut::slice_mesh(mesh_handle->mesh, {config_handle->layer_height});
  if (!sliced.valid()) { fail(sliced.errors.front()); return nullptr; }
  auto* result = new ResultHandle;
  result->layers.resize(sliced.layers.size());
  for (std::size_t i = 0; i < sliced.layers.size(); ++i) {
    const auto& layer = sliced.layers[i];
    layer_cut::Vec2 min = {layer.min.x, layer.min.y};
    layer_cut::Vec2 max = {layer.max.x, layer.max.y};
    if (config_handle->canvas_width > 0.0) {
      min = {0.0, 0.0}; max = {config_handle->canvas_width, config_handle->canvas_height};
    }
    auto contours = layer.contours;
#ifdef LAYER_CUT_HAVE_CLIPPER
    const auto cleanup = layer_cut::apply_manufacturing_cleanup(contours, [&] {
      auto options = config_handle->cleanup;
      options.mode = static_cast<layer_cut::CleanupMode>(config_handle->cleanup_mode);
      return options;
    }());
    if (!cleanup.ok) { delete result; fail(cleanup.error); return nullptr; }
    contours = cleanup.contours;
    result->warnings.insert(result->warnings.end(), cleanup.warnings.begin(), cleanup.warnings.end());
#endif
    if (config_handle->format == SLICER_FORMAT_SVG) {
      result->layers[i].svg = layer_cut::make_svg(contours,
          {min, max, layer.index, layer.z});
    } else {
      const auto png = layer_cut::make_png(contours, {min, max, config_handle->dpi});
      if (!png.ok()) { delete result; fail(png.error); return nullptr; }
      result->layers[i].png = png.bytes;
    }
  }
  return result;
}

int slicer_result_layer_count(slicer_result_t result) {
  if (!valid_handle(result)) { fail("Result is null"); return 0; }
  return static_cast<int>(handle<ResultHandle>(result)->layers.size());
}

const char* slicer_result_layer_svg(slicer_result_t result, int index) {
  if (!valid_handle(result) || index < 0 || static_cast<std::size_t>(index) >= handle<ResultHandle>(result)->layers.size()) {
    fail("SVG layer index is out of range"); return nullptr;
  }
  return handle<ResultHandle>(result)->layers[static_cast<std::size_t>(index)].svg.c_str();
}

const uint8_t* slicer_result_layer_png(slicer_result_t result, int index, size_t* size) {
  if (size) *size = 0;
  if (!valid_handle(result) || index < 0 || static_cast<std::size_t>(index) >= handle<ResultHandle>(result)->layers.size()) {
    fail("PNG layer index is out of range"); return nullptr;
  }
  const auto& bytes = handle<ResultHandle>(result)->layers[static_cast<std::size_t>(index)].png;
  if (size) *size = bytes.size();
  return bytes.empty() ? nullptr : bytes.data();
}

int slicer_result_warning_count(slicer_result_t result) {
  if (!valid_handle(result)) { fail("Result is null"); return 0; }
  return static_cast<int>(handle<ResultHandle>(result)->warnings.size());
}

const char* slicer_result_warning(slicer_result_t result, int index) {
  if (!valid_handle(result) || index < 0 ||
      static_cast<std::size_t>(index) >= handle<ResultHandle>(result)->warnings.size()) {
    fail("Warning index is out of range"); return nullptr;
  }
  return handle<ResultHandle>(result)->warnings[static_cast<std::size_t>(index)].c_str();
}

const char* slicer_last_error(void) { return last_error.c_str(); }
void slicer_free_config(slicer_config_t config) { delete handle<ConfigHandle>(config); }
void slicer_free_mesh(slicer_mesh_t mesh) { delete handle<MeshHandle>(mesh); }
void slicer_free_result(slicer_result_t result) { delete handle<ResultHandle>(result); }

}  // extern "C"
