#include "polygon_ops.h"

#include <cmath>
#include <vector>

namespace {

layer_cut::Contour rectangle(double min_x, double min_y, double max_x,
                             double max_y) {
  return {{{min_x, min_y}, {max_x, min_y}, {max_x, max_y}, {min_x, max_y}},
          false, (max_x - min_x) * (max_y - min_y)};
}

double total_area(const std::vector<layer_cut::Contour>& contours) {
  double area = 0.0;
  for (const auto& contour : contours) area += contour.signed_area;
  return area;
}

}  // namespace

int main() {
  int failures = 0;
  const auto first = rectangle(0, 0, 10, 10);
  const auto second = rectangle(5, 0, 15, 10);
  const auto outer = rectangle(0, 0, 10, 10);
  const auto inner = rectangle(2, 2, 8, 8);

  const auto united = layer_cut::union_polygons({first, second});
  if (!united.ok() || united.contours.size() != 1 ||
      std::abs(total_area(united.contours) - 150.0) > 1e-6) {
    ++failures;
  }

  const auto difference = layer_cut::difference_polygons({outer}, {inner});
  if (!difference.ok() || difference.contours.size() != 2 ||
      std::abs(total_area(difference.contours) - 64.0) > 1e-6) {
    ++failures;
  }

  const auto intersection = layer_cut::intersection_polygons({first}, {second});
  if (!intersection.ok() || intersection.contours.size() != 1 ||
      std::abs(total_area(intersection.contours) - 50.0) > 1e-6) {
    ++failures;
  }

  const auto filtered = layer_cut::remove_small_contours(
      {rectangle(0, 0, 10, 10), rectangle(0, 0, 0.01, 0.01)}, 0.01);
  if (filtered.size() != 1 || std::abs(filtered[0].signed_area) < 1.0) {
    ++failures;
  }
  return failures;
}
