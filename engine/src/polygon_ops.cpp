#include "polygon_ops.h"

#include <clipper2/clipper.h>

#include <cmath>
#include <cstdint>
#include <limits>

namespace layer_cut {

namespace {

constexpr double kScale = 1'000'000.0;

bool to_path(const Contour& contour, Clipper2Lib::Path64& path) {
  constexpr double max_coordinate =
      static_cast<double>(std::numeric_limits<int64_t>::max() >> 2);
  path.clear();
  if (contour.points.size() < 3) return true;
  for (const Vec2& point : contour.points) {
    if (!std::isfinite(point.x) || !std::isfinite(point.y) ||
        std::abs(point.x) > max_coordinate / kScale ||
        std::abs(point.y) > max_coordinate / kScale) {
      return false;
    }
    path.push_back({static_cast<int64_t>(std::llround(point.x * kScale)),
                    static_cast<int64_t>(std::llround(point.y * kScale))});
  }
  return true;
}

bool to_paths(const std::vector<Contour>& contours,
              Clipper2Lib::Paths64& paths) {
  for (const Contour& contour : contours) {
    Clipper2Lib::Path64 path;
    if (!to_path(contour, path)) return false;
    if (!path.empty()) paths.push_back(std::move(path));
  }
  return true;
}

PolygonOperationResult from_paths(const Clipper2Lib::Paths64& paths) {
  std::vector<PlaneSegment> segments;
  for (const auto& path : paths) {
    if (path.size() < 3) continue;
    for (std::size_t i = 0; i < path.size(); ++i) {
      const auto& a = path[i];
      const auto& b = path[(i + 1) % path.size()];
      segments.push_back({{static_cast<double>(a.x) / kScale,
                           static_cast<double>(a.y) / kScale},
                          {static_cast<double>(b.x) / kScale,
                           static_cast<double>(b.y) / kScale},
                          0});
    }
  }
  const ContourBuildResult built = build_contours(segments);
  return {built.contours, built.has_diagnostics() ? "Boolean output was not closed" : ""};
}

PolygonOperationResult boolean_operation(
    Clipper2Lib::ClipType type, const std::vector<Contour>& subjects,
    const std::vector<Contour>& clips) {
  Clipper2Lib::Paths64 subject_paths;
  Clipper2Lib::Paths64 clip_paths;
  if (!to_paths(subjects, subject_paths) || !to_paths(clips, clip_paths)) {
    return {{}, "Polygon coordinate is non-finite or exceeds the integer range"};
  }
  try {
    return from_paths(Clipper2Lib::BooleanOp(
        type, Clipper2Lib::FillRule::NonZero, subject_paths, clip_paths));
  } catch (const Clipper2Lib::Clipper2Exception& exception) {
    return {{}, exception.what()};
  }
}

}  // namespace

PolygonOperationResult union_polygons(const std::vector<Contour>& subjects) {
  return boolean_operation(Clipper2Lib::ClipType::Union, subjects, {});
}

PolygonOperationResult difference_polygons(
    const std::vector<Contour>& subjects, const std::vector<Contour>& clips) {
  return boolean_operation(Clipper2Lib::ClipType::Difference, subjects, clips);
}

PolygonOperationResult intersection_polygons(
    const std::vector<Contour>& subjects, const std::vector<Contour>& clips) {
  return boolean_operation(Clipper2Lib::ClipType::Intersection, subjects, clips);
}

}  // namespace layer_cut
