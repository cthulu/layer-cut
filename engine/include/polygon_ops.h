#pragma once

#include "contour_builder.h"

#include <string>
#include <vector>

namespace layer_cut {

struct PolygonOperationResult {
  std::vector<Contour> contours;
  std::string error;
  bool ok() const { return error.empty(); }
};

enum class CleanupMode { PRESERVE, WARN, APPLY };

struct ManufacturingCleanupOptions {
  CleanupMode mode = CleanupMode::WARN;
  double minimum_feature_width = 0.0;
  double minimum_island_area = 0.0;
  double minimum_hole_width = 0.0;
  double minimum_bridge_width = 0.0;
};

struct ManufacturingCleanupResult {
  std::vector<Contour> contours;
  std::vector<std::string> warnings;
  bool changed = false;
  bool ok = true;
  std::string error;
};

PolygonOperationResult union_polygons(const std::vector<Contour>& subjects);
PolygonOperationResult difference_polygons(
    const std::vector<Contour>& subjects, const std::vector<Contour>& clips);
PolygonOperationResult intersection_polygons(
    const std::vector<Contour>& subjects, const std::vector<Contour>& clips);

// Remove contours whose absolute area is below minimum_area in square mm.
// This is an output cleanup step and must not be used to repair open contours.
std::vector<Contour> remove_small_contours(
    const std::vector<Contour>& contours, double minimum_area);

ManufacturingCleanupResult apply_manufacturing_cleanup(
    const std::vector<Contour>& contours,
    const ManufacturingCleanupOptions& options = ManufacturingCleanupOptions());

}  // namespace layer_cut
