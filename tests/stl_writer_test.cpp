#include "stl_writer.h"

#include <cmath>
#include <string>
#include <vector>

namespace {

layer_cut::Triangle triangle(float z) {
  return {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, z},
          {1.0f, 0.0f, z}, {0.0f, 1.0f, z}};
}

}  // namespace

int main() {
  std::string error;
  const std::string document =
      layer_cut::make_ascii_stl({triangle(0.0f)}, "test_mesh", &error);
  if (!error.empty() || document.find("solid test_mesh\n") == std::string::npos ||
      document.find("facet normal 0.000000000 0.000000000 1.000000000") ==
          std::string::npos ||
      document.find("vertex 1.000000000 0.000000000 0.000000000") ==
          std::string::npos ||
      document.find("endsolid test_mesh\n") == std::string::npos) {
    return 1;
  }

  error.clear();
  if (!layer_cut::make_ascii_stl({}, "empty", &error).empty() || error.empty()) {
    return 2;
  }

  error.clear();
  const layer_cut::Triangle degenerate = {
      {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
      {1.0f, 0.0f, 0.0f}, {2.0f, 0.0f, 0.0f}};
  if (!layer_cut::make_ascii_stl({degenerate}, "degenerate", &error).empty() ||
      error.find("degenerate") == std::string::npos) {
    return 3;
  }

  error.clear();
  layer_cut::Triangle nonfinite = triangle(0.0f);
  nonfinite.a.x = NAN;
  if (!layer_cut::make_ascii_stl({nonfinite}, "nonfinite", &error).empty() ||
      error.find("non-finite") == std::string::npos) {
    return 4;
  }

  return 0;
}
