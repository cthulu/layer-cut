#include "stacked_preview.h"
#include "stl_writer.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

layer_cut::SliceLayer square_layer(std::size_t index, double z) {
  layer_cut::SliceLayer layer;
  layer.index = index;
  layer.z = z;
  layer.contours.push_back({{{0, 0}, {10, 0}, {10, 10}, {0, 10}}, false,
                            100.0});
  return layer;
}

}  // namespace

int main() {
  const auto preview = layer_cut::make_stacked_preview({square_layer(0, 0.5)}, 1.0);
  if (!preview.ok() || preview.triangles.size() != 12) return 1;

  float minimum_z = 100.0f;
  float maximum_z = -100.0f;
  for (const auto& triangle : preview.triangles) {
    for (const auto& point : {triangle.a, triangle.b, triangle.c}) {
      minimum_z = std::min(minimum_z, point.z);
      maximum_z = std::max(maximum_z, point.z);
    }
  }
  if (std::abs(minimum_z) > 1e-6f || std::abs(maximum_z - 1.0f) > 1e-6f) {
    return 2;
  }

  layer_cut::SliceLayer holed = square_layer(1, 1.5);
  holed.contours.push_back({{{3, 3}, {3, 7}, {7, 7}, {7, 3}}, true, -16.0});
  const auto holed_preview = layer_cut::make_stacked_preview({holed}, 1.0);
  if (!holed_preview.ok() || holed_preview.triangles.empty()) return 3;

  double top_area = 0.0;
  for (const auto& triangle : holed_preview.triangles) {
    if (std::abs(triangle.a.z - 2.0f) > 1e-6f ||
        std::abs(triangle.b.z - 2.0f) > 1e-6f ||
        std::abs(triangle.c.z - 2.0f) > 1e-6f) {
      continue;
    }
    top_area += std::abs((triangle.b.x - triangle.a.x) *
                             (triangle.c.y - triangle.a.y) -
                         (triangle.b.y - triangle.a.y) *
                             (triangle.c.x - triangle.a.x)) *
                0.5;
  }
  if (std::abs(top_area - 84.0) > 1e-5) return 4;

  std::string error;
  if (layer_cut::make_ascii_stl(holed_preview.triangles, "preview", &error).empty() ||
      !error.empty()) {
    return 5;
  }
  return 0;
}
