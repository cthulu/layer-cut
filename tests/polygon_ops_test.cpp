#include "polygon_ops.h"

#include <cmath>
#include <iostream>
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

#define CHECK(condition)                                                      \
  do {                                                                        \
    if (!(condition)) {                                                       \
      std::cerr << __LINE__ << ": " #condition "\n";                         \
      ++failures;                                                             \
    }                                                                         \
  } while (false)

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
    CHECK(false);
  }

  const auto difference = layer_cut::difference_polygons({outer}, {inner});
  if (!difference.ok() || difference.contours.size() != 2 ||
      std::abs(total_area(difference.contours) - 64.0) > 1e-6) {
    CHECK(false);
  }

  const auto intersection = layer_cut::intersection_polygons({first}, {second});
  if (!intersection.ok() || intersection.contours.size() != 1 ||
      std::abs(total_area(intersection.contours) - 50.0) > 1e-6) {
    CHECK(false);
  }

  const auto filtered = layer_cut::remove_small_contours(
      {rectangle(0, 0, 10, 10), rectangle(0, 0, 0.01, 0.01)}, 0.01);
  if (filtered.size() != 1 || std::abs(filtered[0].signed_area) < 1.0) {
    CHECK(false);
  }

  const auto narrow = rectangle(0, 0, 1, 10);
  const layer_cut::ManufacturingCleanupOptions warn_options{
      layer_cut::CleanupMode::WARN, 2.0, 0.0, 0.0, 0.0};
  const auto warned = layer_cut::apply_manufacturing_cleanup({narrow}, warn_options);
  if (!warned.ok || warned.changed || warned.contours.size() != 1 || warned.warnings.empty()) CHECK(false);

  const layer_cut::ManufacturingCleanupOptions preserve_options{
      layer_cut::CleanupMode::PRESERVE, 2.0, 0.0, 0.0, 0.0};
  const auto preserved = layer_cut::apply_manufacturing_cleanup({narrow}, preserve_options);
  if (!preserved.ok || preserved.changed || !preserved.warnings.empty() || preserved.contours.size() != 1) CHECK(false);

  const layer_cut::ManufacturingCleanupOptions apply_options{
      layer_cut::CleanupMode::APPLY, 2.0, 0.0, 0.0, 0.0};
  const auto applied = layer_cut::apply_manufacturing_cleanup({narrow}, apply_options);
  if (!applied.ok || !applied.changed || !applied.contours.empty()) CHECK(false);

  const layer_cut::ManufacturingCleanupOptions valid_square_options{
      layer_cut::CleanupMode::WARN, 0.9, 0.0, 0.0, 0.0};
  const auto valid_square = layer_cut::apply_manufacturing_cleanup(
      {rectangle(0, 0, 1, 1)}, valid_square_options);
  if (!valid_square.ok || valid_square.changed || valid_square.warnings.empty()) CHECK(false);

  const auto donut = layer_cut::difference_polygons({rectangle(0, 0, 10, 10)},
                                                    {rectangle(4, 4, 6, 6)});
  const layer_cut::ManufacturingCleanupOptions close_options{
      layer_cut::CleanupMode::APPLY, 0.0, 0.0, 3.0, 0.0};
  const auto closed = layer_cut::apply_manufacturing_cleanup(donut.contours, close_options);
  if (!donut.ok() || donut.contours.size() != 2 || !closed.ok || !closed.changed ||
      closed.contours.size() != 1 || closed.contours[0].hole) {
    std::cerr << "donut=" << donut.ok() << ',' << donut.contours.size()
              << " closed=" << closed.ok << ',' << closed.changed << ','
              << closed.contours.size();
    if (!closed.contours.empty()) std::cerr << ',' << closed.contours[0].hole;
    std::cerr << '\n';
    CHECK(false);
  }
  return failures;
}
