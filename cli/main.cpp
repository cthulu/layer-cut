#include "layer_cut.h"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
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

void print_progress(int current_layer, int total_layers) {
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

void engine_progress(void* context, int current_layer, int total_layers,
                     double fraction) {
  (void)context;
  (void)fraction;
  print_progress(current_layer, total_layers);
}

std::string zero_padded(int value) {
  const std::string digits = std::to_string(value);
  return digits.size() >= 3 ? digits : std::string(3 - digits.size(), '0') + digits;
}

// Releases engine handles on every exit path. The free functions accept
// null handles, so partially initialized guards are safe.
struct EngineHandles {
  slicer_mesh_t mesh = nullptr;
  slicer_config_t config = nullptr;
  slicer_result_t result = nullptr;
  ~EngineHandles() {
    slicer_free_result(result);
    slicer_free_config(config);
    slicer_free_mesh(mesh);
  }
};

bool write_text_file(const std::filesystem::path& path, const char* contents) {
  std::ofstream output(path, std::ios::binary);
  return contents != nullptr && output && (output << contents);
}

bool write_binary_file(const std::filesystem::path& path, const uint8_t* data,
                       std::size_t size) {
  std::ofstream output(path, std::ios::binary);
  return data != nullptr && output &&
         output.write(reinterpret_cast<const char*>(data),
                      static_cast<std::streamsize>(size));
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
                 "Optional stacked-layer preview STL path (binary). The "
                 "preview uses physical layer thickness in millimetres to "
                 "approximate the assembled design");
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

  EngineHandles handles;
  handles.mesh = slicer_load_stl(stl_path.c_str());
  if (!handles.mesh) {
    std::cerr << "Error: " << slicer_last_error() << "\n";
    return 1;
  }
  const int diagnostic_count = slicer_mesh_diagnostic_count(handles.mesh);
  if (diagnostic_count > 0) {
    std::cerr << "Warning: " << diagnostic_count
              << " mesh diagnostic(s); continuing best-effort.\n";
  }

  const std::array<std::string, 6> axis_names = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
  const int cutting_axis = static_cast<int>(std::distance(
      axis_names.begin(), std::find(axis_names.begin(), axis_names.end(), cutting_axis_name)));
  const int format_code = format == "png"             ? SLICER_FORMAT_PNG
                          : format == "cricut-normal" ? SLICER_FORMAT_CRICUT_NORMAL
                          : format == "cricut-large"  ? SLICER_FORMAT_CRICUT_LARGE
                                                      : SLICER_FORMAT_SVG;
  const int cleanup_code = cleanup_mode == "preserve" ? 0
                           : cleanup_mode == "apply"  ? 2
                                                      : 1;

  handles.config = slicer_config_create();
  bool config_ok =
      handles.config != nullptr &&
      slicer_config_set_layer_height(handles.config, layer_height) &&
      slicer_config_set_format(handles.config, format_code) &&
      slicer_config_set_dpi(handles.config, dpi) &&
      slicer_config_set_cricut_gap(handles.config, cricut_gap) &&
      slicer_config_set_cricut_packing(handles.config,
                                       cricut_packing == "tight" ? 1 : 0) &&
      slicer_config_set_cricut_guide_inset(handles.config, cricut_guide_inset) &&
      slicer_config_set_show_layer_numbers(handles.config,
                                           layer_numbers ? 1 : 0) &&
      slicer_config_set_layer_number_font_size(handles.config,
                                               layer_number_font_size) &&
      slicer_config_set_cleanup_mode(handles.config, cleanup_code) &&
      slicer_config_set_cleanup_thresholds(
          handles.config, minimum_feature_width, minimum_area,
          minimum_hole_width, minimum_bridge_width) &&
      slicer_config_set_transform_euler(handles.config, cutting_axis, rotate_x,
                                        rotate_y, rotate_z, transform_scale) &&
      slicer_config_set_progress_callback(handles.config, engine_progress,
                                          nullptr);
  if (config_ok && canvas_width > 0.0) {
    config_ok = slicer_config_set_canvas(handles.config, canvas_width,
                                         canvas_height) != 0;
  }
  if (!config_ok) {
    std::cerr << "Error: " << slicer_last_error() << "\n";
    return 2;
  }

  handles.result = slicer_slice(handles.mesh, handles.config);
  if (!handles.result) {
    std::cerr << "Error: " << slicer_last_error() << "\n";
    return 2;
  }

  std::error_code filesystem_error;
  std::filesystem::create_directories(output_dir, filesystem_error);
  if (filesystem_error) {
    std::cerr << "Error: Cannot create output directory: " << output_dir << "\n";
    return 1;
  }

  const int warning_count = slicer_result_warning_count(handles.result);
  for (int i = 0; i < warning_count; ++i) {
    const char* warning = slicer_result_warning(handles.result, i);
    if (warning != nullptr) std::cerr << "Warning: " << warning << "\n";
  }

  const bool cricut_format =
      format == "cricut-normal" || format == "cricut-large";
  int exported = 0;
  if (cricut_format) {
    const int page_count = slicer_result_page_count(handles.result);
    for (int page = 0; page < page_count; ++page) {
      const int layer_start = slicer_result_page_layer_start(handles.result, page);
      const int layer_end =
          layer_start + slicer_result_page_layer_count(handles.result, page) - 1;
      const std::string prefix = "page_" + zero_padded(page + 1) + "_layers_" +
                                 zero_padded(layer_start) + "-" +
                                 zero_padded(layer_end);
      bool written = false;
      if (cricut_combined) {
        written = write_text_file(
            std::filesystem::path(output_dir) / (prefix + ".svg"),
            slicer_result_page_combined_svg(handles.result, page));
      } else {
        written = write_text_file(
                      std::filesystem::path(output_dir) / (prefix + ".cut.svg"),
                      slicer_result_page_cut_svg(handles.result, page)) &&
                  write_text_file(
                      std::filesystem::path(output_dir) / (prefix + ".guide.svg"),
                      slicer_result_page_guide_svg(handles.result, page));
      }
      if (!written) {
        std::cerr << "Error: Cannot write Cricut page " << (page + 1) << "\n";
        return 1;
      }
      exported += slicer_result_page_layer_count(handles.result, page);
    }
  } else {
    const int layer_count = slicer_result_layer_count(handles.result);
    for (int index = 0; index < layer_count; ++index) {
      const std::filesystem::path filename =
          std::filesystem::path(output_dir) /
          ("layer_" + zero_padded(index) + "." + format);
      bool written = false;
      if (format == "svg") {
        written =
            write_text_file(filename, slicer_result_layer_svg(handles.result, index));
      } else {
        std::size_t size = 0;
        const uint8_t* png =
            slicer_result_layer_png(handles.result, index, &size);
        written = write_binary_file(filename, png, size);
      }
      if (!written) {
        std::cerr << "Error: Cannot write layer output: " << filename << "\n";
        return 1;
      }
      ++exported;
    }
  }

  if (!stacked_stl_path.empty()) {
    std::size_t size = 0;
    const uint8_t* data = slicer_result_stacked_stl(handles.result, &size);
    if (data == nullptr || size < 84) {
      std::cerr << "Error: Stacked STL generation failed";
      for (int i = 0; i < slicer_result_diagnostic_count(handles.result); ++i) {
        const char* diagnostic = slicer_result_diagnostic(handles.result, i);
        if (diagnostic != nullptr &&
            std::string(diagnostic).find("Stacked preview") != std::string::npos) {
          std::cerr << ": " << diagnostic;
        }
      }
      std::cerr << "\n";
      return 1;
    }
    if (!write_binary_file(stacked_stl_path, data, size)) {
      std::cerr << "Error: Cannot write stacked STL output: " << stacked_stl_path
                << "\n";
      return 1;
    }
    uint32_t triangle_count = 0;
    std::memcpy(&triangle_count, data + 80, sizeof(triangle_count));
    std::cout << "Generated stacked STL with " << triangle_count
              << " triangle(s) at " << stacked_stl_path << "\n";
  }

  std::cout << "Loaded " << slicer_mesh_triangle_count(handles.mesh)
            << " triangles from " << stl_path << "\n";
  if (cricut_format) {
    std::cout << "Generated " << slicer_result_page_count(handles.result)
              << " Cricut page(s) in " << output_dir << "\n";
  } else {
    std::cout << "Generated " << exported << " " << format << " layer(s) in "
              << output_dir << "\n";
  }
  return 0;
}
