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

PolygonOperationResult union_polygons(const std::vector<Contour>& subjects);
PolygonOperationResult difference_polygons(
    const std::vector<Contour>& subjects, const std::vector<Contour>& clips);
PolygonOperationResult intersection_polygons(
    const std::vector<Contour>& subjects, const std::vector<Contour>& clips);

}  // namespace layer_cut
