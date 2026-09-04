#pragma once

#include "contour_builder.h"

#include <cstddef>
#include <string>
#include <vector>

namespace layer_cut {

struct SvgOptions {
  Vec2 min{0.0, 0.0};
  Vec2 max{0.0, 0.0};
  std::size_t layer_index = 0;
  double layer_z = 0.0;
};

std::string make_svg(const std::vector<Contour>& contours,
                     const SvgOptions& options);

bool write_svg_file(const std::string& path, const std::vector<Contour>& contours,
                    const SvgOptions& options, std::string* error = nullptr);

}  // namespace layer_cut
