#pragma once

#include "contour_builder.h"

#include <cstddef>
#include <string>
#include <vector>

namespace layer_cut {

struct SliceConfig {
  double layer_height = 0.2;
  double epsilon = 1e-6;
};

struct SliceLayer {
  std::size_t index = 0;
  double z = 0.0;
  Vec2 min{0.0, 0.0};
  Vec2 max{0.0, 0.0};
  std::vector<Contour> contours;
  std::vector<ContourDiagnostic> diagnostics;
};

struct SliceResult {
  std::vector<SliceLayer> layers;
  std::vector<std::string> errors;
  bool valid() const { return errors.empty(); }
};

SliceResult slice_mesh(const Mesh& mesh, const SliceConfig& config = SliceConfig());

}  // namespace layer_cut
