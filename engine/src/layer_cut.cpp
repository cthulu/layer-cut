#include "layer_cut.h"

#include "mesh.h"
#include "polygon_ops.h"
#include "png_writer.h"
#include "cricut_page.h"
#include "slicer.h"
#include "stl_loader.h"
#include "stacked_preview.h"
#include "stl_writer.h"
#include "svg_writer.h"
#include "transform.h"

#include <cmath>
#include <atomic>
#include <algorithm>
#include <memory>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
thread_local std::string last_error;

struct MeshHandle {
  layer_cut::Mesh mesh;
  std::vector<layer_cut::MeshDiagnostic> diagnostics;
};
struct ConfigHandle {
  double layer_height = 0.2;
  double canvas_width = 0.0;
  double canvas_height = 0.0;
  int format = SLICER_FORMAT_SVG;
  int dpi = 300;
  double cricut_gap = 3.0;
  int cricut_packing = 0;
  double cricut_guide_inset = 1.0;
  int show_layer_numbers = 0;
  double layer_number_font_size_mm = 2.5;
  int cleanup_mode = 1;
  int layer_preview = 0;
  layer_cut::ManufacturingCleanupOptions cleanup;
  slicer_progress_callback_t progress_callback = nullptr;
  void* progress_context = nullptr;
  int axis = 4;
  double rotate_x_degrees = 0.0;
  double rotate_y_degrees = 0.0;
  double rotate_z_degrees = 0.0;
  double scale = 1.0;
  std::shared_ptr<std::atomic<bool>> cancellation;
};
struct LayerBytes {
  std::string svg;
  std::string preview_svg;
  std::vector<uint8_t> png;
  double z = 0.0;
  bool empty = true;
};
struct PageBytes {
  std::string cut_svg;
  std::string guide_svg;
  std::string combined_svg;
  int layer_start = 0;
  int layer_count = 0;
  int path_count = 0;
};
struct ResultHandle {
  std::vector<LayerBytes> layers;
  std::vector<PageBytes> pages;
  std::vector<std::string> warnings;
  std::vector<std::string> diagnostics;
  std::vector<int> diagnostic_severity;
  std::string stacked_stl;
};

void append_preview_paths(std::ostringstream& output,
                          const std::vector<layer_cut::Contour>& contours,
                          const char* indent) {
  for (const auto& contour : contours) {
    if (contour.points.size() < 3) continue;
    output << indent << "<path d=\"M " << contour.points[0].x << ' '
           << contour.points[0].y;
    for (std::size_t i = 1; i < contour.points.size(); ++i) {
      output << " L " << contour.points[i].x << ' ' << contour.points[i].y;
    }
    output << " Z\"/>\n";
  }
}

std::string make_layer_preview_svg(const std::vector<layer_cut::Contour>& current,
                                   const std::vector<layer_cut::Contour>* next,
                                   const layer_cut::Vec2& min,
                                   const layer_cut::Vec2& max,
                                   std::size_t index, double z,
                                   double inset, std::vector<std::string>* warnings) {
  std::vector<layer_cut::Contour> guide;
  if (next != nullptr && !next->empty()) {
    guide = *next;
#ifdef LAYER_CUT_HAVE_CLIPPER
    if (std::isfinite(inset) && inset > 0.0) {
      const auto offset = layer_cut::offset_polygons(*next, -inset);
      const auto outside = offset.ok()
                               ? layer_cut::difference_polygons(offset.contours, current)
                               : layer_cut::PolygonOperationResult{};
      if (offset.ok() && outside.ok() && outside.contours.empty() && !offset.contours.empty()) {
        guide = offset.contours;
      } else if (warnings) {
        warnings->push_back("Layer preview inset invalid or not contained for layer " +
                            std::to_string(index + 1) + "; using outline");
      }
    } else if (warnings) {
      warnings->push_back("Layer preview inset must be positive; using outline");
    }
#else
    if (warnings) warnings->push_back("Layer preview inset unavailable without Clipper2");
#endif
  }
  const double width = max.x - min.x;
  const double height = max.y - min.y;
  std::ostringstream output;
  output << std::fixed << std::setprecision(9)
         << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\""
         << min.x << ' ' << min.y << ' ' << width << ' ' << height
         << "\" width=\"" << width << "mm\" height=\"" << height
         << "mm\" data-layer-index=\"" << index << "\" data-layer-z=\"" << z << "\">\n"
         << "  <g id=\"cut-layer\" fill=\"black\" fill-rule=\"evenodd\" stroke=\"none\">\n";
  append_preview_paths(output, current, "    ");
  output << "  </g>\n";
  if (!guide.empty()) {
    output << "  <g id=\"next-layer-guide\" fill=\"none\" stroke=\"#1769aa\" stroke-width=\"0.25\">\n";
    append_preview_paths(output, guide, "    ");
    output << "  </g>\n";
  }
  output << "</svg>\n";
  return output.str();
}
struct SnapshotHandle {
  std::vector<float> vertices;
  std::vector<uint32_t> indices;
  slicer_bounds_t bounds{};
};
struct CancellationHandle {
  std::shared_ptr<std::atomic<bool>> value =
      std::make_shared<std::atomic<bool>>(false);
};

template <typename T> T* handle(void* value) { return static_cast<T*>(value); }
void fail(std::string message) { last_error = std::move(message); }
bool valid_handle(const void* value) { return value != nullptr; }

bool cancelled(const ConfigHandle& config) {
  return config.cancellation && config.cancellation->load();
}
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
  result->diagnostics = validation.diagnostics;
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
    if (!valid_handle(config) ||
        (format != SLICER_FORMAT_SVG && format != SLICER_FORMAT_PNG &&
         format != SLICER_FORMAT_CRICUT_NORMAL &&
         format != SLICER_FORMAT_CRICUT_LARGE)) {
    fail("Unsupported output format"); return 0;
  }
  handle<ConfigHandle>(config)->format = format; return 1;
}

int slicer_config_set_dpi(slicer_config_t config, int dpi) {
  if (!valid_handle(config) || dpi <= 0) { fail("DPI must be positive"); return 0; }
  handle<ConfigHandle>(config)->dpi = dpi; return 1;
}

int slicer_config_set_cricut_gap(slicer_config_t config, double gap_mm) {
  if (!valid_handle(config) || !std::isfinite(gap_mm)) {
    fail("Cricut gap must be finite");
    return 0;
  }
  handle<ConfigHandle>(config)->cricut_gap = gap_mm;
  return 1;
}

int slicer_config_set_cricut_packing(slicer_config_t config, int tight) {
  if (!valid_handle(config) || (tight != 0 && tight != 1)) {
    fail("Invalid Cricut packing strategy");
    return 0;
  }
  handle<ConfigHandle>(config)->cricut_packing = tight;
  return 1;
}

int slicer_config_set_cricut_guide_inset(slicer_config_t config, double inset_mm) {
  if (!valid_handle(config) || !std::isfinite(inset_mm) || inset_mm <= 0.0) {
    fail("Cricut guide inset must be positive and finite");
    return 0;
  }
  handle<ConfigHandle>(config)->cricut_guide_inset = inset_mm;
  return 1;
}

int slicer_config_set_show_layer_numbers(slicer_config_t config, int enabled) {
  if (!valid_handle(config) || (enabled != 0 && enabled != 1)) {
    fail("Layer numbering must be 0 or 1");
    return 0;
  }
  handle<ConfigHandle>(config)->show_layer_numbers = enabled;
  return 1;
}

int slicer_config_set_layer_number_font_size(slicer_config_t config, double size_mm) {
  if (!valid_handle(config) || !std::isfinite(size_mm) || size_mm <= 0.0) {
    fail("Layer number font size must be positive and finite");
    return 0;
  }
  handle<ConfigHandle>(config)->layer_number_font_size_mm = size_mm;
  return 1;
}

int slicer_config_set_cleanup_mode(slicer_config_t config, int mode) {
  if (!valid_handle(config) || mode < 0 || mode > 2) {
    fail("Cleanup mode must be 0 (preserve), 1 (warn), or 2 (apply)"); return 0;
  }
  handle<ConfigHandle>(config)->cleanup_mode = mode; return 1;
}

int slicer_config_set_layer_preview(slicer_config_t config, int enabled) {
  if (!valid_handle(config) || (enabled != 0 && enabled != 1)) {
    fail("Layer preview must be 0 or 1");
    return 0;
  }
  handle<ConfigHandle>(config)->layer_preview = enabled;
  return 1;
}

int slicer_config_set_progress_callback(slicer_config_t config,
                                        slicer_progress_callback_t callback,
                                        void* context) {
  if (!valid_handle(config)) {
    fail("Config is required");
    return 0;
  }
  auto* config_handle = handle<ConfigHandle>(config);
  config_handle->progress_callback = callback;
  config_handle->progress_context = context;
  return 1;
}

int slicer_config_set_transform(slicer_config_t config, int axis,
                                double rotation_degrees, double scale) {
  return slicer_config_set_transform_euler(config, axis, 0.0, 0.0,
                                           rotation_degrees, scale);
}

int slicer_config_set_transform_euler(slicer_config_t config, int axis,
                                      double rotate_x_degrees,
                                      double rotate_y_degrees,
                                      double rotate_z_degrees, double scale) {
  if (!valid_handle(config) || axis < 0 || axis > 5 ||
      !std::isfinite(rotate_x_degrees) || rotate_x_degrees < -180.0 ||
      rotate_x_degrees > 180.0 || !std::isfinite(rotate_y_degrees) ||
      rotate_y_degrees < -180.0 || rotate_y_degrees > 180.0 ||
      !std::isfinite(rotate_z_degrees) || rotate_z_degrees < -180.0 ||
      rotate_z_degrees > 180.0 || !std::isfinite(scale) || scale <= 0.0) {
    fail("Transform values are invalid");
    return 0;
  }
  auto* value = handle<ConfigHandle>(config);
  value->axis = axis;
  value->rotate_x_degrees = rotate_x_degrees;
  value->rotate_y_degrees = rotate_y_degrees;
  value->rotate_z_degrees = rotate_z_degrees;
  value->scale = scale;
  return 1;
}

int slicer_config_set_cancellation(slicer_config_t config,
                                   slicer_cancellation_t cancellation) {
  if (!valid_handle(config)) { fail("Config is required"); return 0; }
  handle<ConfigHandle>(config)->cancellation = cancellation
      ? handle<CancellationHandle>(cancellation)->value : nullptr;
  return 1;
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
  if (cancelled(*config_handle)) { fail("Slicing cancelled"); return nullptr; }
  const auto prepared_mesh = layer_cut::transform_mesh(
      mesh_handle->mesh, {config_handle->axis, config_handle->rotate_x_degrees,
                          config_handle->rotate_y_degrees,
                          config_handle->rotate_z_degrees, config_handle->scale});
  const auto sliced = layer_cut::slice_mesh(prepared_mesh, {config_handle->layer_height});
  if (!sliced.valid()) { fail(sliced.errors.front()); return nullptr; }
  const int total_layers = static_cast<int>(sliced.layers.size());
  if (config_handle->progress_callback) {
    config_handle->progress_callback(config_handle->progress_context, 0,
                                     total_layers, total_layers == 0 ? 1.0 : 0.0);
  }
  auto* result = new ResultHandle;
  for (const auto& diagnostic : mesh_handle->diagnostics) {
    result->diagnostics.push_back(diagnostic.message);
    result->diagnostic_severity.push_back(
        diagnostic.severity == layer_cut::DiagnosticSeverity::ERROR ? 1 : 0);
  }

  if (config_handle->format == SLICER_FORMAT_CRICUT_NORMAL ||
      config_handle->format == SLICER_FORMAT_CRICUT_LARGE) {
#ifndef LAYER_CUT_HAVE_CLIPPER
    delete result;
    fail("Cricut page output requires Clipper2");
    return nullptr;
#else
    std::vector<layer_cut::SliceLayer> prepared_layers;
    prepared_layers.reserve(sliced.layers.size());
    for (const auto& layer : sliced.layers) {
      if (cancelled(*config_handle)) { delete result; fail("Slicing cancelled"); return nullptr; }
      const auto unioned = layer_cut::union_polygons(layer.contours);
      if (!unioned.ok()) {
        delete result;
        fail(unioned.error);
        return nullptr;
      }
      auto cleanup = config_handle->cleanup;
      cleanup.mode = static_cast<layer_cut::CleanupMode>(config_handle->cleanup_mode);
      const auto cleaned = layer_cut::apply_manufacturing_cleanup(unioned.contours, cleanup);
      if (!cleaned.ok) {
        delete result;
        fail(cleaned.error);
        return nullptr;
      }
      result->warnings.insert(result->warnings.end(), cleaned.warnings.begin(),
                              cleaned.warnings.end());
      auto prepared = layer;
      prepared.contours = cleaned.contours;
      prepared_layers.push_back(std::move(prepared));
    }
    layer_cut::CricutPageOptions page_options;
    page_options.size = config_handle->format == SLICER_FORMAT_CRICUT_LARGE
                            ? layer_cut::CricutPageSize::LARGE
                            : layer_cut::CricutPageSize::NORMAL;
    page_options.gap_mm = config_handle->cricut_gap;
    page_options.packing = config_handle->cricut_packing
                               ? layer_cut::CricutPackingStrategy::TIGHT
                               : layer_cut::CricutPackingStrategy::FIXED;
    page_options.show_layer_numbers = config_handle->show_layer_numbers != 0;
    page_options.layer_number_font_size_mm = config_handle->layer_number_font_size_mm;
    const auto pages = layer_cut::build_cricut_pages(prepared_layers, page_options);
    if (!pages.ok()) {
      delete result;
      fail(pages.error);
      return nullptr;
    }
    result->layers.resize(prepared_layers.size());
    for (std::size_t i = 0; i < prepared_layers.size(); ++i) {
      const auto& layer = prepared_layers[i];
      result->layers[i].z = layer.z;
      result->layers[i].empty = layer.contours.empty();
      result->layers[i].svg = layer_cut::make_svg(
          layer.contours,
          {{layer.min.x, layer.min.y}, {layer.max.x, layer.max.y},
           layer.index, layer.z});
    }
    // Preview composition is opt-in; generating it unconditionally more than
    // halves export/CLI throughput on Cricut formats.
    if (config_handle->layer_preview != 0) for (std::size_t i = 0; i < prepared_layers.size(); ++i) {
      const auto& layer = prepared_layers[i];
      const auto* next = i + 1 < prepared_layers.size()
                             ? &prepared_layers[i + 1].contours
                             : nullptr;
      result->layers[i].preview_svg = make_layer_preview_svg(
          layer.contours, next, layer.min, layer.max, layer.index, layer.z,
          config_handle->cricut_guide_inset, &result->warnings);
    }
    result->pages.reserve(pages.pages.size());
    for (const auto& page : pages.pages) {
      PageBytes bytes;
      bytes.cut_svg = layer_cut::make_cricut_cut_svg(page);
      std::vector<std::string> guide_warnings;
      bytes.guide_svg = layer_cut::make_cricut_guide_svg(
          page, config_handle->cricut_guide_inset, &guide_warnings);
      bytes.combined_svg = layer_cut::make_cricut_combined_svg(
          page, config_handle->cricut_guide_inset, &guide_warnings);
      result->warnings.insert(result->warnings.end(), guide_warnings.begin(),
                              guide_warnings.end());
      bytes.layer_start = static_cast<int>(page.tiles.front().layer_index);
      bytes.layer_count = static_cast<int>(page.tiles.size());
      bytes.path_count = static_cast<int>(page.cut_path_count);
      result->pages.push_back(std::move(bytes));
    }
    if (config_handle->progress_callback) {
      config_handle->progress_callback(config_handle->progress_context,
                                       total_layers, total_layers, 1.0);
    }
    const auto stacked = layer_cut::make_stacked_preview(prepared_layers,
                                                         config_handle->layer_height);
    if (stacked.ok()) {
      result->stacked_stl = layer_cut::make_binary_stl(stacked.triangles);
    } else if (!stacked.error.empty()) {
      result->diagnostics.push_back("Stacked preview: " + stacked.error);
      result->diagnostic_severity.push_back(0);
    }
    return result;
#endif
  }

  result->layers.resize(sliced.layers.size());
  std::vector<std::vector<layer_cut::Contour>> preview_contours;
  preview_contours.reserve(sliced.layers.size());
  for (std::size_t i = 0; i < sliced.layers.size(); ++i) {
    if (cancelled(*config_handle)) { delete result; fail("Slicing cancelled"); return nullptr; }
    const auto& layer = sliced.layers[i];
    result->layers[i].z = layer.z;
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
    if (config_handle->layer_preview != 0) {
      preview_contours.push_back(contours);
    }
    result->layers[i].empty = contours.empty();
    // Keep vector layer data available for the macOS preview. PNG is an export
    // format; Cricut formats still produce SVG page data and need SVG layers.
    if (config_handle->format != SLICER_FORMAT_PNG) {
      result->layers[i].svg = layer_cut::make_svg(contours,
          {min, max, layer.index, layer.z});
    } else {
      const auto png = layer_cut::make_png(contours, {min, max, config_handle->dpi});
      if (!png.ok()) { delete result; fail(png.error); return nullptr; }
      result->layers[i].png = png.bytes;
    }
    if (config_handle->progress_callback) {
      const int current_layer = static_cast<int>(i + 1);
      config_handle->progress_callback(
          config_handle->progress_context, current_layer, total_layers,
          total_layers == 0 ? 1.0
                            : static_cast<double>(current_layer) / total_layers);
    }
  }
  if (config_handle->layer_preview != 0) for (std::size_t i = 0; i < sliced.layers.size(); ++i) {
    const auto& layer = sliced.layers[i];
    const auto* next = i + 1 < preview_contours.size() ? &preview_contours[i + 1] : nullptr;
    result->layers[i].preview_svg = make_layer_preview_svg(
        preview_contours[i], next, layer.min, layer.max, layer.index, layer.z,
        config_handle->cricut_guide_inset, &result->warnings);
  }
  const auto stacked = layer_cut::make_stacked_preview(sliced.layers,
                                                       config_handle->layer_height);
  if (stacked.ok()) {
    result->stacked_stl = layer_cut::make_binary_stl(stacked.triangles);
  } else if (!stacked.error.empty()) {
    result->diagnostics.push_back("Stacked preview: " + stacked.error);
    result->diagnostic_severity.push_back(0);
  }
  return result;
}

int slicer_result_layer_count(slicer_result_t result) {
  if (!valid_handle(result)) { fail("Result is null"); return 0; }
  return static_cast<int>(handle<ResultHandle>(result)->layers.size());
}

double slicer_result_layer_z(slicer_result_t result, int index) {
  if (!valid_handle(result) || index < 0 ||
      static_cast<std::size_t>(index) >= handle<ResultHandle>(result)->layers.size()) {
    fail("Layer index is out of range");
    return 0.0;
  }
  return handle<ResultHandle>(result)->layers[static_cast<std::size_t>(index)].z;
}

int slicer_result_layer_is_empty(slicer_result_t result, int index) {
  if (!valid_handle(result) || index < 0 ||
      static_cast<std::size_t>(index) >= handle<ResultHandle>(result)->layers.size()) {
    fail("Layer index is out of range");
    return 1;
  }
  return handle<ResultHandle>(result)->layers[static_cast<std::size_t>(index)].empty ? 1 : 0;
}

const char* slicer_result_layer_svg(slicer_result_t result, int index) {
  if (!valid_handle(result) || index < 0 || static_cast<std::size_t>(index) >= handle<ResultHandle>(result)->layers.size()) {
    fail("SVG layer index is out of range"); return nullptr;
  }
  const auto& svg = handle<ResultHandle>(result)->layers[static_cast<std::size_t>(index)].svg;
  return svg.empty() ? nullptr : svg.c_str();
}

size_t slicer_result_layer_svg_size(slicer_result_t result, int index) {
  const char* value = slicer_result_layer_svg(result, index);
  return value == nullptr ? 0 : handle<ResultHandle>(result)->layers[static_cast<std::size_t>(index)].svg.size();
}

const char* slicer_result_layer_preview_svg(slicer_result_t result, int index) {
  if (!valid_handle(result) || index < 0 ||
      static_cast<std::size_t>(index) >= handle<ResultHandle>(result)->layers.size()) {
    fail("Layer preview SVG index is out of range"); return nullptr;
  }
  const auto& svg = handle<ResultHandle>(result)->layers[static_cast<std::size_t>(index)].preview_svg;
  return svg.empty() ? nullptr : svg.c_str();
}

size_t slicer_result_layer_preview_svg_size(slicer_result_t result, int index) {
  const char* value = slicer_result_layer_preview_svg(result, index);
  return value == nullptr ? 0 : handle<ResultHandle>(result)->layers[static_cast<std::size_t>(index)].preview_svg.size();
}

int slicer_result_page_count(slicer_result_t result) {
  if (!valid_handle(result)) {
    fail("Result is null");
    return 0;
  }
  return static_cast<int>(handle<ResultHandle>(result)->pages.size());
}

PageBytes* page_at(slicer_result_t result, int index) {
  if (!valid_handle(result) || index < 0 ||
      static_cast<std::size_t>(index) >= handle<ResultHandle>(result)->pages.size()) {
    fail("Cricut page index is out of range");
    return nullptr;
  }
  return &handle<ResultHandle>(result)->pages[static_cast<std::size_t>(index)];
}

const char* slicer_result_page_cut_svg(slicer_result_t result, int page) {
  PageBytes* value = page_at(result, page);
  return value == nullptr ? nullptr : value->cut_svg.c_str();
}

size_t slicer_result_page_cut_svg_size(slicer_result_t result, int page) {
  PageBytes* value = page_at(result, page);
  return value == nullptr ? 0 : value->cut_svg.size();
}

const char* slicer_result_page_guide_svg(slicer_result_t result, int page) {
  PageBytes* value = page_at(result, page);
  return value == nullptr ? nullptr : value->guide_svg.c_str();
}

size_t slicer_result_page_guide_svg_size(slicer_result_t result, int page) {
  PageBytes* value = page_at(result, page);
  return value == nullptr ? 0 : value->guide_svg.size();
}

const char* slicer_result_page_combined_svg(slicer_result_t result, int page) {
  PageBytes* value = page_at(result, page);
  return value == nullptr ? nullptr : value->combined_svg.c_str();
}

size_t slicer_result_page_combined_svg_size(slicer_result_t result, int page) {
  PageBytes* value = page_at(result, page);
  return value == nullptr ? 0 : value->combined_svg.size();
}

int slicer_result_page_layer_start(slicer_result_t result, int page) {
  PageBytes* value = page_at(result, page);
  return value == nullptr ? -1 : value->layer_start;
}

int slicer_result_page_layer_count(slicer_result_t result, int page) {
  PageBytes* value = page_at(result, page);
  return value == nullptr ? 0 : value->layer_count;
}

int slicer_result_page_path_count(slicer_result_t result, int page) {
  PageBytes* value = page_at(result, page);
  return value == nullptr ? 0 : value->path_count;
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
  const auto& warning = handle<ResultHandle>(result)->warnings[static_cast<std::size_t>(index)];
  return warning.empty() ? nullptr : warning.c_str();
}

size_t slicer_result_warning_size(slicer_result_t result, int index) {
  const char* value = slicer_result_warning(result, index);
  return value == nullptr ? 0 : handle<ResultHandle>(result)->warnings[static_cast<std::size_t>(index)].size();
}

const uint8_t* slicer_result_stacked_stl(slicer_result_t result, size_t* size) {
  if (size) *size = 0;
  if (!valid_handle(result)) { fail("Result is null"); return nullptr; }
  const auto& value = handle<ResultHandle>(result)->stacked_stl;
  if (size) *size = value.size();
  return value.empty() ? nullptr : reinterpret_cast<const uint8_t*>(value.data());
}

int slicer_result_diagnostic_count(slicer_result_t result) {
  return valid_handle(result) ? static_cast<int>(handle<ResultHandle>(result)->diagnostics.size()) : 0;
}

const char* slicer_result_diagnostic(slicer_result_t result, int index) {
  if (!valid_handle(result) || index < 0 ||
      static_cast<std::size_t>(index) >= handle<ResultHandle>(result)->diagnostics.size()) {
    fail("Diagnostic index is out of range"); return nullptr;
  }
  return handle<ResultHandle>(result)->diagnostics[static_cast<std::size_t>(index)].c_str();
}

int slicer_result_diagnostic_severity(slicer_result_t result, int index) {
  if (!valid_handle(result) || index < 0 ||
      static_cast<std::size_t>(index) >= handle<ResultHandle>(result)->diagnostic_severity.size()) {
    fail("Diagnostic index is out of range"); return -1;
  }
  return handle<ResultHandle>(result)->diagnostic_severity[static_cast<std::size_t>(index)];
}

int slicer_mesh_triangle_count(slicer_mesh_t mesh) {
  return valid_handle(mesh) ? static_cast<int>(handle<MeshHandle>(mesh)->mesh.triangles.size()) : 0;
}

int slicer_mesh_bounds(slicer_mesh_t mesh, slicer_bounds_t* bounds) {
  if (!valid_handle(mesh) || !bounds) { fail("Mesh bounds request is invalid"); return 0; }
  const auto& value = handle<MeshHandle>(mesh)->mesh;
  *bounds = {value.min.x, value.min.y, value.min.z, value.max.x, value.max.y, value.max.z};
  return 1;
}

double slicer_mesh_volume(slicer_mesh_t mesh) {
  if (!valid_handle(mesh)) { fail("Mesh is null"); return 0.0; }
  return handle<MeshHandle>(mesh)->mesh.compute_volume();
}

int slicer_mesh_diagnostic_count(slicer_mesh_t mesh) {
  return valid_handle(mesh) ? static_cast<int>(handle<MeshHandle>(mesh)->diagnostics.size()) : 0;
}

const char* slicer_mesh_diagnostic(slicer_mesh_t mesh, int index) {
  if (!valid_handle(mesh) || index < 0 ||
      static_cast<std::size_t>(index) >= handle<MeshHandle>(mesh)->diagnostics.size()) {
    fail("Mesh diagnostic index is out of range"); return nullptr;
  }
  return handle<MeshHandle>(mesh)->diagnostics[static_cast<std::size_t>(index)].message.c_str();
}

int slicer_mesh_diagnostic_severity(slicer_mesh_t mesh, int index) {
  if (!valid_handle(mesh) || index < 0 ||
      static_cast<std::size_t>(index) >= handle<MeshHandle>(mesh)->diagnostics.size()) {
    fail("Mesh diagnostic index is out of range"); return -1;
  }
  return handle<MeshHandle>(mesh)->diagnostics[static_cast<std::size_t>(index)].severity ==
             layer_cut::DiagnosticSeverity::ERROR ? 1 : 0;
}

int slicer_mesh_snapshot(slicer_mesh_t mesh, int axis, double rotation_degrees,
                         double scale, slicer_snapshot_t* snapshot) {
  return slicer_mesh_snapshot_euler(mesh, axis, 0.0, 0.0, rotation_degrees,
                                    scale, snapshot);
}

int slicer_mesh_snapshot_euler(slicer_mesh_t mesh, int axis,
                               double rotate_x_degrees,
                               double rotate_y_degrees,
                               double rotate_z_degrees, double scale,
                               slicer_snapshot_t* snapshot) {
  if (snapshot) *snapshot = nullptr;
  if (!valid_handle(mesh) || !snapshot || axis < 0 || axis > 5 ||
       !std::isfinite(rotate_x_degrees) || rotate_x_degrees < -180.0 ||
       rotate_x_degrees > 180.0 || !std::isfinite(rotate_y_degrees) ||
       rotate_y_degrees < -180.0 || rotate_y_degrees > 180.0 ||
       !std::isfinite(rotate_z_degrees) || rotate_z_degrees < -180.0 ||
       rotate_z_degrees > 180.0 || !std::isfinite(scale) || scale <= 0.0) {
    fail("Invalid mesh snapshot request"); return 0;
  }
  const auto value = layer_cut::transform_mesh(
      handle<MeshHandle>(mesh)->mesh,
      {axis, rotate_x_degrees, rotate_y_degrees, rotate_z_degrees, scale});
  auto* output = new SnapshotHandle;
  output->vertices.reserve(value.triangles.size() * 9);
  output->indices.reserve(value.triangles.size() * 3);
  for (const auto& triangle : value.triangles) {
    for (const auto& point : {triangle.a, triangle.b, triangle.c}) {
      output->indices.push_back(static_cast<uint32_t>(output->vertices.size() / 3));
      output->vertices.insert(output->vertices.end(), {point.x, point.y, point.z});
    }
  }
  output->bounds = {value.min.x, value.min.y, value.min.z,
                    value.max.x, value.max.y, value.max.z};
  *snapshot = output;
  return 1;
}

const float* slicer_snapshot_vertices(slicer_snapshot_t snapshot, size_t* count) {
  if (count) *count = 0;
  if (!valid_handle(snapshot)) return nullptr;
  const auto& values = handle<SnapshotHandle>(snapshot)->vertices;
  if (count) *count = values.size();
  return values.empty() ? nullptr : values.data();
}

const uint32_t* slicer_snapshot_indices(slicer_snapshot_t snapshot, size_t* count) {
  if (count) *count = 0;
  if (!valid_handle(snapshot)) return nullptr;
  const auto& values = handle<SnapshotHandle>(snapshot)->indices;
  if (count) *count = values.size();
  return values.empty() ? nullptr : values.data();
}

int slicer_snapshot_bounds(slicer_snapshot_t snapshot, slicer_bounds_t* bounds) {
  if (!valid_handle(snapshot) || !bounds) { fail("Snapshot bounds request is invalid"); return 0; }
  *bounds = handle<SnapshotHandle>(snapshot)->bounds;
  return 1;
}

slicer_cancellation_t slicer_cancellation_create(void) { return new CancellationHandle; }
void slicer_cancellation_cancel(slicer_cancellation_t cancellation) {
  if (valid_handle(cancellation)) handle<CancellationHandle>(cancellation)->value->store(true);
}
int slicer_cancellation_is_cancelled(slicer_cancellation_t cancellation) {
  return valid_handle(cancellation) && handle<CancellationHandle>(cancellation)->value->load();
}

const char* slicer_last_error(void) { return last_error.c_str(); }
void slicer_free_config(slicer_config_t config) { delete handle<ConfigHandle>(config); }
void slicer_free_mesh(slicer_mesh_t mesh) { delete handle<MeshHandle>(mesh); }
void slicer_free_result(slicer_result_t result) { delete handle<ResultHandle>(result); }
void slicer_free_snapshot(slicer_snapshot_t snapshot) { delete handle<SnapshotHandle>(snapshot); }
void slicer_free_cancellation(slicer_cancellation_t cancellation) { delete handle<CancellationHandle>(cancellation); }

}  // extern "C"
