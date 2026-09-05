#pragma once

#include "contour_builder.h"

#include <cstdint>
#include <string>
#include <vector>

namespace layer_cut {

struct PngOptions {
  Vec2 min{0.0, 0.0};
  Vec2 max{0.0, 0.0};
  int dpi = 300;
  bool black_on_white = true;
  bool antialias = true;
};

struct PngResult {
  std::vector<std::uint8_t> bytes;
  int width = 0;
  int height = 0;
  std::string error;
  bool ok() const { return error.empty(); }
};

PngResult make_png(const std::vector<Contour>& contours, const PngOptions& options);

}  // namespace layer_cut
