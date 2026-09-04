#include "stl_loader.h"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
  CLI::App app{"layer-cut — STL slicer for 2D cutting machines"};
  app.footer(
      "Conventions:\n"
      "  STL coordinates are interpreted as millimetres.\n"
      "  The input Z axis is sliced with horizontal XY planes.\n"
      "  Slicing normalizes the model by translating min-Z to 0; rotation is\n"
      "  not performed.\n");

  std::string output_dir = "output";
  double layer_height = 0.2;
  std::string format = "svg";
  int dpi = 300;
  double canvas_width = 0.0;
  double canvas_height = 0.0;
  std::string stl_path;

  app.add_option("-o,--output-dir", output_dir,
                 "Output directory for generated layers (default: output)");

  app.add_option("-l,--layer-height", layer_height,
                 "Layer height in millimetres (default: 0.2)")
      ->check(CLI::Range(0.01, 1.0));

  app.add_option("-f,--format", format,
                 "Output format: svg or png (default: svg)")
      ->check(CLI::IsMember({"svg", "png"}));

  app.add_option("-d,--dpi", dpi,
                 "PNG dots per inch (default: 300)")
      ->check(CLI::Range(72, 1200));

  app.add_option("-w,--canvas-width", canvas_width,
                 "Optional output canvas width in millimetres");

  app.add_option("-H,--canvas-height", canvas_height,
                 "Optional output canvas height in millimetres");

  app.add_option("stl-file", stl_path,
                 "Input STL file path (required)")
      ->required();

  try {
    app.parse(argc, argv);
  } catch (const CLI::Error& e) {
    if (e.get_exit_code() == 0) {
      std::cout << app.help();
      return 0;
    }
    std::cerr << "Error: " << e.what() << "\n";
    std::cout << "\n" << app.help();
    return 2;
  }

  // Validate canvas dimensions
  if ((canvas_width > 0.0) != (canvas_height > 0.0)) {
    std::cerr << "Error: --canvas-width and --canvas-height must both be "
                 "specified or both omitted.\n";
    return 2;
  }
  if ((canvas_width != 0.0 &&
       (!std::isfinite(canvas_width) || canvas_width <= 0.0)) ||
      (canvas_height != 0.0 &&
       (!std::isfinite(canvas_height) || canvas_height <= 0.0))) {
    std::cerr << "Error: canvas dimensions must be positive finite values.\n";
    return 2;
  }

  // Load STL
  layer_cut::LoadError error;
  auto triangles = layer_cut::load_stl(stl_path, layer_cut::LoadLimits(), &error);

  if (error.code != layer_cut::LoadError::Code::OK) {
    switch (error.code) {
      case layer_cut::LoadError::Code::FILE_NOT_FOUND:
        std::cerr << "Error: File not found: " << stl_path << "\n";
        break;
      case layer_cut::LoadError::Code::NOT_BINARY_STL:
        std::cerr << "Error: Not a binary STL file: " << stl_path << "\n";
        break;
      case layer_cut::LoadError::Code::TRUNCATED:
        std::cerr << "Error: File is truncated: " << stl_path << "\n";
        break;
      case layer_cut::LoadError::Code::TOO_MANY_TRIANGLES:
        std::cerr << "Error: File exceeds triangle limit: " << stl_path << "\n";
        break;
      case layer_cut::LoadError::Code::NONFINITE_COORDINATE:
        std::cerr << "Error: File contains non-finite coordinates: "
                  << stl_path << "\n";
        break;
      case layer_cut::LoadError::Code::INVALID_FORMAT:
        std::cerr << "Error: Invalid file format: " << stl_path << "\n";
        break;
      case layer_cut::LoadError::Code::OK:
        std::cerr << "Error: Unknown error loading: " << stl_path << "\n";
        break;
    }
    return 1;
  }

  if (triangles.empty()) {
    std::cerr << "Error: STL contains no triangles.\n";
    return 2;
  }

  // Print model metadata
  std::cout << "Loaded " << triangles.size() << " triangles from " << stl_path
            << "\n";

  if (!triangles.empty()) {
    float min_x = triangles[0].a.x, min_y = triangles[0].a.y,
          min_z = triangles[0].a.z;
    float max_x = triangles[0].a.x, max_y = triangles[0].a.y,
          max_z = triangles[0].a.z;

    for (const auto& tri : triangles) {
      min_x = std::min(min_x, std::min({tri.a.x, tri.b.x, tri.c.x}));
      min_y = std::min(min_y, std::min({tri.a.y, tri.b.y, tri.c.y}));
      min_z = std::min(min_z, std::min({tri.a.z, tri.b.z, tri.c.z}));
      max_x = std::max(max_x, std::max({tri.a.x, tri.b.x, tri.c.x}));
      max_y = std::max(max_y, std::max({tri.a.y, tri.b.y, tri.c.y}));
      max_z = std::max(max_z, std::max({tri.a.z, tri.b.z, tri.c.z}));
    }

    std::cout << "  Bounds: [" << min_x << ", " << min_y << ", " << min_z
              << "] to [" << max_x << ", " << max_y << ", " << max_z << "] mm\n";
    std::cout << "  Z range: " << (max_z - min_z) << " mm\n";
  }

  // Print configuration summary
  std::cout << "\nConfiguration:\n";
  std::cout << "  Output directory: " << output_dir << "\n";
  std::cout << "  Layer height: " << layer_height << " mm\n";
  std::cout << "  Format: " << format << "\n";
  std::cout << "  DPI: " << dpi << "\n";
  if (canvas_width > 0.0 && canvas_height > 0.0) {
    std::cout << "  Canvas: " << canvas_width << " x " << canvas_height
              << " mm\n";
  }

  std::cout << "\nNote: Slicing and export are not yet implemented.\n";

  return 0;
}
