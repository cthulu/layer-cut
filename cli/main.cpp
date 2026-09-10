#include "polygon_ops.h"
#include "slicer.h"
#include "stl_loader.h"
#include "svg_writer.h"
#include "png_writer.h"
#include "stacked_preview.h"
#include "stl_writer.h"
#include "cricut_page.h"
#include "transform.h"

#include <CLI/CLI.hpp>

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <array>
#include <cmath>
#include <string>
#ifdef _WIN32
#include <io.h>
#define LAYER_CUT_ISATTY _isatty
#define LAYER_CUT_FILENO _fileno
#else
#include <unistd.h>
#define LAYER_CUT_ISATTY isatty
#define LAYER_CUT_FILENO fileno
#endif

namespace {

void print_progress(std::size_t current_layer, std::size_t total_layers) {
  static constexpr char spinner[] = {'|', '/', '-', '\\'};
  static std::size_t update = 0;
  const double fraction = total_layers == 0
                              ? 1.0
                              : static_cast<double>(current_layer) / total_layers;
  const int percent = static_cast<int>(std::lround(
      std::clamp(fraction, 0.0, 1.0) * 100.0));
  constexpr int bar_width = 24;
  const int filled = static_cast<int>(std::lround(fraction * bar_width));
  std::string bar(static_cast<std::size_t>(filled), '#');
  bar.append(static_cast<std::size_t>(bar_width - filled), '-');
  const bool interactive = LAYER_CUT_ISATTY(LAYER_CUT_FILENO(stderr)) != 0;
  if (interactive) {
    std::cerr << "\r" << spinner[update++ % 4] << " Layer " << current_layer
              << " of " << total_layers << " [" << bar << "] "
              << std::setw(3) << percent << "%" << std::flush;
    if (current_layer >= total_layers) std::cerr << '\n';
  } else {
    std::cerr << "Layer " << current_layer << " of " << total_layers << " ["
              << bar << "] " << std::setw(3) << percent << "%\n";
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  CLI::App app{"layer-cut - STL slicer for 2D cutting machines"};
  app.footer(
      "Conventions:\n"
      "  STL coordinates are interpreted as millimetres.\n"
      "  The input Z axis is sliced with horizontal XY planes.\n"
      "  Transform order is scale * axis orientation * Rz * Ry * Rx * input.\n"
      "  Slicing translates min-Z to 0.\n");

  std::string output_dir = "output";
  double layer_height = 0.2;
  std::string format = "svg";
  int dpi = 300;
  double canvas_width = 0.0;
  double canvas_height = 0.0;
  double minimum_area = 0.01;
  std::string cleanup_mode = "warn";
  double minimum_feature_width = 0.0;
  double minimum_hole_width = 0.0;
  double minimum_bridge_width = 0.0;
  double cricut_gap = 3.0;
  double cricut_guide_inset = 1.0;
  std::string cricut_packing = "fixed";
  bool cricut_combined = false;
  bool layer_numbers = false;
  double layer_number_font_size = 2.5;
  std::string stl_path;
  std::string stacked_stl_path;
  std::string cutting_axis_name = "+Z";
  double rotate_x = 0.0, rotate_y = 0.0, rotate_z = 0.0, transform_scale = 1.0;

  app.add_option("-o,--output-dir", output_dir,
                 "Output directory for generated layers (default: output)");
  app.add_option("-l,--layer-height", layer_height,
                 "Layer height in millimetres (default: 0.2)")
      ->check(CLI::Range(0.01, 6.0));
  app.add_option("-f,--format", format,
                 "Output format: svg, png, cricut-normal, or cricut-large")
      ->check(CLI::IsMember({"svg", "png", "cricut-normal", "cricut-large"}));
  app.add_option("-d,--dpi", dpi, "PNG dots per inch (default: 300)")
      ->check(CLI::Range(72, 1200));
  app.add_option("-w,--canvas-width", canvas_width,
                 "Optional output canvas width in millimetres");
  app.add_option("-H,--canvas-height", canvas_height,
                 "Optional output canvas height in millimetres");
  app.add_option("--min-area", minimum_area,
                 "Minimum island area in square mm (legacy alias; 0 disables)")
      ->check(CLI::Range(0.0, 1000000.0));
  app.add_option("--cleanup", cleanup_mode,
                 "Manufacturing cleanup: preserve, warn, or apply (default: warn)")
      ->check(CLI::IsMember({"preserve", "warn", "apply"}));
  app.add_option("--min-feature-width", minimum_feature_width,
                 "Minimum feature width in millimetres")
      ->check(CLI::Range(0.0, 1000000.0));
  app.add_option("--min-hole-width", minimum_hole_width,
                 "Minimum hole width in millimetres")
      ->check(CLI::Range(0.0, 1000000.0));
  app.add_option("--min-bridge-width", minimum_bridge_width,
                 "Minimum bridge width in millimetres")
      ->check(CLI::Range(0.0, 1000000.0));
  app.add_option("--cricut-gap", cricut_gap,
                 "Cricut tile gap in millimetres (default: 3)");
  app.add_option("--packing", cricut_packing,
                 "Cricut packing strategy: fixed or tight (default: fixed)")
      ->check(CLI::IsMember({"fixed", "tight"}));
  app.add_option("--cricut-guide-inset", cricut_guide_inset,
                 "Cricut guide inset in millimetres (default: 1)");
  app.add_flag("--cricut-combined", cricut_combined,
               "Write one SVG per Cricut page with cut and pen groups");
  app.add_flag("--layer-numbers", layer_numbers,
               "Add layer numbers to combined Cricut SVG pages");
  app.add_option("--layer-number-font-size", layer_number_font_size,
                 "Layer number font size in millimetres (default: 2.5)")
      ->check(CLI::Range(0.01, 100.0));
  app.add_option("--stacked-stl", stacked_stl_path,
                 "Optional watertight stacked-layer preview STL path");
  app.add_option("--cutting-axis", cutting_axis_name,
                 "Cutting axis: +X, -X, +Y, -Y, +Z, or -Z (default: +Z)")
      ->check(CLI::IsMember({"+X", "-X", "+Y", "-Y", "+Z", "-Z"}));
  app.add_option("--rotate-x", rotate_x,
                 "Rotate around X in degrees (default: 0)")
      ->check(CLI::Range(-180.0, 180.0));
  app.add_option("--rotate-y", rotate_y,
                 "Rotate around Y in degrees (default: 0)")
      ->check(CLI::Range(-180.0, 180.0));
  app.add_option("--rotate-z", rotate_z,
                 "Rotate around Z in degrees (default: 0)")
      ->check(CLI::Range(-180.0, 180.0));
  app.add_option("--scale", transform_scale,
                 "Uniform positive model scale (default: 1)")
      ->check(CLI::Range(0.000001, 1000000.0));
  app.add_option("stl-file", stl_path, "Input STL file path (required)")
      ->required();

  try {
    app.parse(argc, argv);
  } catch (const CLI::Error& e) {
    if (e.get_exit_code() == 0) {
      std::cout << app.help();
      return 0;
    }
    std::cerr << "Error: " << e.what() << "\n\n" << app.help();
    return 2;
  }

  if ((canvas_width > 0.0) != (canvas_height > 0.0) ||
      (canvas_width != 0.0 && canvas_width <= 0.0) ||
      (canvas_height != 0.0 && canvas_height <= 0.0)) {
    std::cerr << "Error: canvas dimensions must both be positive or omitted.\n";
    return 2;
  }
  if (!std::isfinite(rotate_x) || !std::isfinite(rotate_y) ||
      !std::isfinite(rotate_z) || rotate_x < -180.0 || rotate_x > 180.0 ||
      rotate_y < -180.0 || rotate_y > 180.0 || rotate_z < -180.0 ||
      rotate_z > 180.0 || !std::isfinite(transform_scale) ||
      transform_scale <= 0.0) {
    std::cerr << "Error: transform angles must be finite in [-180, 180] and "
                 "scale must be positive and finite.\n";
    return 2;
  }
  if (cricut_combined && format != "cricut-normal" && format != "cricut-large") {
    std::cerr << "Error: --cricut-combined requires a Cricut page format.\n";
    return 2;
  }
  if (layer_numbers && !cricut_combined) {
    std::cerr << "Error: --layer-numbers requires --cricut-combined.\n";
    return 2;
  }

  layer_cut::LoadError load_error;
  const auto triangles = layer_cut::load_stl(stl_path, {}, &load_error);
  if (load_error.code != layer_cut::LoadError::Code::OK) {
    std::cerr << "Error: " << load_error.message << "\n";
    return 1;
  }
  const auto validation = layer_cut::validate_and_normalize(triangles);
  if (validation.has_errors()) {
    for (const auto& diagnostic : validation.diagnostics) {
      std::cerr << "Error: " << diagnostic.message << "\n";
    }
    return 2;
  }
  if (!validation.diagnostics.empty()) {
    std::cerr << "Warning: " << validation.diagnostics.size()
              << " mesh diagnostic(s); continuing best-effort.\n";
  }

  const std::array<std::string, 6> axis_names = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
  const int cutting_axis = static_cast<int>(std::distance(
      axis_names.begin(), std::find(axis_names.begin(), axis_names.end(), cutting_axis_name)));
  const auto transformed = layer_cut::transform_mesh(validation.mesh,
      {cutting_axis, rotate_x, rotate_y, rotate_z, transform_scale});
  const auto sliced = layer_cut::slice_mesh(transformed, {layer_height});
  if (!sliced.valid()) {
    for (const std::string& error : sliced.errors) std::cerr << "Error: " << error << "\n";
    return 2;
  }

  std::error_code filesystem_error;
  std::filesystem::create_directories(output_dir, filesystem_error);
  if (filesystem_error) {
    std::cerr << "Error: Cannot create output directory: " << output_dir << "\n";
    return 1;
  }

  std::size_t exported = 0;
  std::vector<layer_cut::SliceLayer> prepared_layers;
  prepared_layers.reserve(sliced.layers.size());
  print_progress(0, sliced.layers.size());

  if (format == "cricut-normal" || format == "cricut-large") {
    std::vector<layer_cut::SliceLayer> prepared_layers;
    prepared_layers.reserve(sliced.layers.size());
    for (const auto& layer : sliced.layers) {
      const auto cleaned = layer_cut::union_polygons(layer.contours);
      if (!cleaned.ok()) {
        std::cerr << "Error: Polygon cleanup failed for layer " << layer.index
                  << ": " << cleaned.error << "\n";
        return 1;
      }
      layer_cut::ManufacturingCleanupOptions cleanup;
      cleanup.mode = cleanup_mode == "preserve"
                         ? layer_cut::CleanupMode::PRESERVE
                         : cleanup_mode == "apply" ? layer_cut::CleanupMode::APPLY
                                                     : layer_cut::CleanupMode::WARN;
      cleanup.minimum_feature_width = minimum_feature_width;
      cleanup.minimum_hole_width = minimum_hole_width;
      cleanup.minimum_bridge_width = minimum_bridge_width;
      cleanup.minimum_island_area = minimum_area;
      const auto cleaned_for_cut =
          layer_cut::apply_manufacturing_cleanup(cleaned.contours, cleanup);
      if (!cleaned_for_cut.ok) {
        std::cerr << "Error: Manufacturing cleanup failed for layer " << layer.index
                  << ": " << cleaned_for_cut.error << "\n";
        return 1;
      }
      auto prepared_layer = layer;
      prepared_layer.contours = cleaned_for_cut.contours;
      prepared_layers.push_back(std::move(prepared_layer));
    }
    layer_cut::CricutPageOptions page_options;
    page_options.size = format == "cricut-large"
                            ? layer_cut::CricutPageSize::LARGE
                            : layer_cut::CricutPageSize::NORMAL;
    page_options.gap_mm = cricut_gap;
    page_options.packing = cricut_packing == "tight"
                               ? layer_cut::CricutPackingStrategy::TIGHT
                               : layer_cut::CricutPackingStrategy::FIXED;
    page_options.show_layer_numbers = layer_numbers;
    page_options.layer_number_font_size_mm = layer_number_font_size;
    const auto pages = layer_cut::build_cricut_pages(prepared_layers, page_options);
    if (!pages.ok()) {
      std::cerr << "Error: Cricut page generation failed: " << pages.error << "\n";
      return 1;
    }
    for (const auto& page : pages.pages) {
      const std::string prefix = "page_" +
          (page.page_index < 1000 ? std::string(3 - std::to_string(page.page_index).size(), '0') : "") +
          std::to_string(page.page_index) + "_layers_" +
          (page.tiles.front().layer_index < 1000 ? std::string(3 - std::to_string(page.tiles.front().layer_index).size(), '0') : "") +
          std::to_string(page.tiles.front().layer_index) + "-" +
          (page.tiles.back().layer_index < 1000 ? std::string(3 - std::to_string(page.tiles.back().layer_index).size(), '0') : "") +
          std::to_string(page.tiles.back().layer_index);
      std::vector<std::string> guide_warnings;
      if (cricut_combined) {
        const auto output_path = std::filesystem::path(output_dir) / (prefix + ".svg");
        std::ofstream output(output_path);
        const std::string combined = layer_cut::make_cricut_combined_svg(
            page, cricut_guide_inset, &guide_warnings);
        if (!output || !(output << combined)) {
          std::cerr << "Error: Cannot write Cricut page " << page.page_index << "\n";
          return 1;
        }
      } else {
        const auto cut_path = std::filesystem::path(output_dir) / (prefix + ".cut.svg");
        const auto guide_path = std::filesystem::path(output_dir) / (prefix + ".guide.svg");
        const std::string guide_svg = layer_cut::make_cricut_guide_svg(
            page, cricut_guide_inset, &guide_warnings);
        std::ofstream cut(cut_path);
        std::ofstream guide(guide_path);
        if (!cut || !guide || !(cut << layer_cut::make_cricut_cut_svg(page)) ||
            !(guide << guide_svg)) {
          std::cerr << "Error: Cannot write Cricut page " << page.page_index << "\n";
          return 1;
        }
      }
      for (const std::string& warning : guide_warnings) {
        std::cerr << "Warning: page " << page.page_index << ": " << warning << "\n";
      }
      exported += page.tiles.size();
    }
    std::cout << "Loaded " << triangles.size() << " triangles from " << stl_path
              << "\nGenerated " << pages.pages.size() << " Cricut page(s) in "
              << output_dir << "\n";
    return 0;
  }

  for (const auto& layer : sliced.layers) {
    const auto cleaned = layer_cut::union_polygons(layer.contours);
    if (!cleaned.ok()) {
      std::cerr << "Error: Polygon cleanup failed for layer " << layer.index
                << ": " << cleaned.error << "\n";
      return 1;
    }
    layer_cut::ManufacturingCleanupOptions cleanup;
    cleanup.mode = cleanup_mode == "preserve"
                       ? layer_cut::CleanupMode::PRESERVE
                       : cleanup_mode == "apply" ? layer_cut::CleanupMode::APPLY
                                                   : layer_cut::CleanupMode::WARN;
    cleanup.minimum_feature_width = minimum_feature_width;
    cleanup.minimum_hole_width = minimum_hole_width;
    cleanup.minimum_bridge_width = minimum_bridge_width;
    cleanup.minimum_island_area = minimum_area;
    const auto cleaned_for_cut =
        layer_cut::apply_manufacturing_cleanup(cleaned.contours, cleanup);
    if (!cleaned_for_cut.ok) {
      std::cerr << "Error: Manufacturing cleanup failed for layer " << layer.index
                << ": " << cleaned_for_cut.error << "\n";
      return 1;
    }
    if (!cleaned_for_cut.warnings.empty()) {
      std::cerr << "Warning: layer " << layer.index << ": "
                << cleaned_for_cut.warnings.size()
                << " feature(s) below manufacturing limits";
      if (cleanup.mode == layer_cut::CleanupMode::APPLY && cleaned_for_cut.changed)
        std::cerr << "; geometry modified";
      std::cerr << "\n";
    }
    const auto& filtered = cleaned_for_cut.contours;
    auto prepared_layer = layer;
    prepared_layer.contours = filtered;
    prepared_layers.push_back(std::move(prepared_layer));
    layer_cut::SvgOptions svg_options;
    svg_options.layer_index = layer.index;
    svg_options.layer_z = layer.z;
    svg_options.min = {canvas_width > 0.0 ? 0.0 : layer.min.x,
                       canvas_height > 0.0 ? 0.0 : layer.min.y};
    svg_options.max = {canvas_width > 0.0 ? canvas_width : layer.max.x,
                       canvas_height > 0.0 ? canvas_height : layer.max.y};
    const std::filesystem::path filename =
        std::filesystem::path(output_dir) /
         ("layer_" + (layer.index < 1000 ? std::string(3 - std::to_string(layer.index).size(), '0') : "") +
          std::to_string(layer.index) + "." + format);
    if (format == "svg") {
      std::string write_error;
      if (!layer_cut::write_svg_file(filename.string(), filtered, svg_options, &write_error)) {
        std::cerr << "Error: " << write_error << "\n";
        return 1;
      }
    } else {
      layer_cut::PngOptions png_options;
      png_options.min = svg_options.min;
      png_options.max = svg_options.max;
      png_options.dpi = dpi;
      const auto png = layer_cut::make_png(filtered, png_options);
      if (!png.ok()) {
        std::cerr << "Error: " << png.error << "\n";
        return 1;
      }
      std::ofstream output(filename, std::ios::binary);
      if (!output || !output.write(reinterpret_cast<const char*>(png.bytes.data()),
                                   static_cast<std::streamsize>(png.bytes.size()))) {
        std::cerr << "Error: Cannot write PNG output: " << filename << "\n";
        return 1;
      }
    }
    ++exported;
    print_progress(exported, sliced.layers.size());
  }

  if (!stacked_stl_path.empty()) {
    const auto preview =
        layer_cut::make_stacked_preview(prepared_layers, layer_height);
    if (!preview.ok()) {
      std::cerr << "Error: Stacked STL generation failed: " << preview.error
                << "\n";
      return 1;
    }
    std::string write_error;
    if (!layer_cut::write_ascii_stl_file(stacked_stl_path, preview.triangles,
                                         "layer_cut_stacked_preview",
                                         &write_error)) {
      std::cerr << "Error: " << write_error << "\n";
      return 1;
    }
    std::cout << "Generated stacked STL with " << preview.triangles.size()
              << " triangle(s) at " << stacked_stl_path << "\n";
  }

  std::cout << "Loaded " << triangles.size() << " triangles from " << stl_path
            << "\nGenerated " << exported << " " << format << " layer(s) in " << output_dir
            << "\n";
  return 0;
}
