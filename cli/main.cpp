#include "polygon_ops.h"
#include "slicer.h"
#include "stl_loader.h"
#include "svg_writer.h"

#include <CLI/CLI.hpp>

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  CLI::App app{"layer-cut - STL slicer for 2D cutting machines"};
  app.footer(
      "Conventions:\n"
      "  STL coordinates are interpreted as millimetres.\n"
      "  The input Z axis is sliced with horizontal XY planes.\n"
      "  Slicing translates min-Z to 0; rotation is not performed.\n");

  std::string output_dir = "output";
  double layer_height = 0.2;
  std::string format = "svg";
  int dpi = 300;
  double canvas_width = 0.0;
  double canvas_height = 0.0;
  double minimum_area = 0.01;
  std::string stl_path;

  app.add_option("-o,--output-dir", output_dir,
                 "Output directory for generated layers (default: output)");
  app.add_option("-l,--layer-height", layer_height,
                 "Layer height in millimetres (default: 0.2)")
      ->check(CLI::Range(0.01, 1.0));
  app.add_option("-f,--format", format,
                 "Output format: svg (png is not implemented yet)")
      ->check(CLI::IsMember({"svg", "png"}));
  app.add_option("-d,--dpi", dpi, "PNG dots per inch (default: 300)")
      ->check(CLI::Range(72, 1200));
  app.add_option("-w,--canvas-width", canvas_width,
                 "Optional output canvas width in millimetres");
  app.add_option("-H,--canvas-height", canvas_height,
                 "Optional output canvas height in millimetres");
  app.add_option("--min-area", minimum_area,
                 "Remove output contours smaller than this area in square mm (default: 0.01; 0 disables)")
      ->check(CLI::Range(0.0, 1000000.0));
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

  if (format != "svg") {
    std::cerr << "Error: PNG output is not implemented yet. Use --format svg.\n";
    return 2;
  }
  if ((canvas_width > 0.0) != (canvas_height > 0.0) ||
      (canvas_width != 0.0 && canvas_width <= 0.0) ||
      (canvas_height != 0.0 && canvas_height <= 0.0)) {
    std::cerr << "Error: canvas dimensions must both be positive or omitted.\n";
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

  const auto sliced = layer_cut::slice_mesh(validation.mesh, {layer_height});
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
  for (const auto& layer : sliced.layers) {
    const auto cleaned = layer_cut::union_polygons(layer.contours);
    if (!cleaned.ok()) {
      std::cerr << "Error: Polygon cleanup failed for layer " << layer.index
                << ": " << cleaned.error << "\n";
      return 1;
    }
    const auto filtered =
        layer_cut::remove_small_contours(cleaned.contours, minimum_area);
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
         std::to_string(layer.index) + ".svg");
    std::string write_error;
    if (!layer_cut::write_svg_file(filename.string(), filtered,
                                   svg_options, &write_error)) {
      std::cerr << "Error: " << write_error << "\n";
      return 1;
    }
    ++exported;
  }

  std::cout << "Loaded " << triangles.size() << " triangles from " << stl_path
            << "\nGenerated " << exported << " SVG layer(s) in " << output_dir
            << "\n";
  return 0;
}
