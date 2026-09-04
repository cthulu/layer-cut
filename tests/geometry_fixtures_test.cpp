#include "contour_builder.h"
#include "mesh.h"
#include "slicer.h"

#include <cmath>
#include <vector>

namespace {
using namespace layer_cut;

Triangle triangle(Vec3 a, Vec3 b, Vec3 c) { return {{0, 0, 0}, a, b, c}; }

std::vector<Triangle> box(float x0, float y0, float z0, float x1, float y1, float z1) {
  return {
      triangle({x0, y0, z0}, {x1, y1, z0}, {x0, y1, z0}),
      triangle({x0, y0, z0}, {x1, y0, z0}, {x1, y1, z0}),
      triangle({x0, y0, z1}, {x0, y1, z1}, {x1, y1, z1}),
      triangle({x0, y0, z1}, {x1, y1, z1}, {x1, y0, z1}),
      triangle({x0, y0, z0}, {x0, y1, z0}, {x0, y1, z1}),
      triangle({x0, y0, z0}, {x0, y1, z1}, {x0, y0, z1}),
      triangle({x1, y0, z0}, {x1, y0, z1}, {x1, y1, z1}),
      triangle({x1, y0, z0}, {x1, y1, z1}, {x1, y1, z0}),
      triangle({x0, y0, z0}, {x0, y0, z1}, {x1, y0, z1}),
      triangle({x0, y0, z0}, {x1, y0, z1}, {x1, y0, z0}),
      triangle({x0, y1, z0}, {x1, y1, z1}, {x0, y1, z1}),
      triangle({x0, y1, z0}, {x1, y1, z0}, {x1, y1, z1}),
  };
}

}  // namespace

int main() {
  int failures = 0;
  {
    const auto validation = validate_and_normalize(box(-10, -10, -5, 10, 10, 15));
    if (validation.has_errors() || validation.mesh.min.z != 0.0f ||
        validation.mesh.max.z != 20.0f || validation.mesh.min.x != -10.0f) ++failures;
    const auto sliced = slice_mesh(validation.mesh, {0.7, 1e-6});
    if (!sliced.valid() || sliced.layers.size() != 29) ++failures;
  }
  {
    auto disconnected = box(0, 0, 0, 2, 2, 2);
    const auto second = box(5, 5, 0, 7, 7, 2);
    disconnected.insert(disconnected.end(), second.begin(), second.end());
    const auto validation = validate_and_normalize(disconnected);
    bool warned = false;
    for (const auto& diagnostic : validation.diagnostics) {
      warned = warned || diagnostic.category == DiagnosticCategory::DISCONNECTED_COMPONENT;
    }
    if (!warned || validation.has_errors()) ++failures;
  }
  {
    const std::vector<PlaneSegment> concave = {
        {{0, 0}, {4, 0}, 0}, {{4, 0}, {4, 4}, 0}, {{4, 4}, {2, 2}, 0},
        {{2, 2}, {0, 4}, 0}, {{0, 4}, {0, 0}, 0}};
    const auto result = build_contours(concave);
    if (result.contours.size() != 1 || std::abs(result.contours[0].signed_area - 12.0) > 1e-9) ++failures;
  }
  {
    const std::vector<PlaneSegment> hole = {
        {{0, 0}, {10, 0}, 0}, {{10, 0}, {10, 10}, 0}, {{10, 10}, {0, 10}, 0},
        {{0, 10}, {0, 0}, 0}, {{2, 2}, {2, 8}, 0}, {{2, 8}, {8, 8}, 0},
        {{8, 8}, {8, 2}, 0}, {{8, 2}, {2, 2}, 0}};
    const auto result = build_contours(hole);
    if (result.contours.size() != 2) ++failures;
  }
  return failures;
}
