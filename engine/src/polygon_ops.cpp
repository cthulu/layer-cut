#include "polygon_ops.h"

#include <clipper2/clipper.h>

#include <algorithm>
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

PolygonOperationResult from_paths(const Clipper2Lib::Paths64& paths);

double perimeter(const Contour& contour) {
  double length = 0.0;
  for (std::size_t i = 0; i < contour.points.size(); ++i) {
    const Vec2& a = contour.points[i];
    const Vec2& b = contour.points[(i + 1) % contour.points.size()];
    length += std::hypot(b.x - a.x, b.y - a.y);
  }
  return length;
}

double estimated_width(const Contour& contour) {
  const double length = perimeter(contour);
  return length > 0.0 ? 2.0 * std::abs(contour.signed_area) / length : 0.0;
}

PolygonOperationResult from_closed_paths(const Clipper2Lib::Paths64& paths) {
  std::vector<Contour> contours;
  for (const auto& path : paths) {
    if (path.size() < 3) continue;
    Contour contour;
    contour.points.reserve(path.size());
    for (const auto& point : path) {
      contour.points.push_back({static_cast<double>(point.x) / kScale,
                                static_cast<double>(point.y) / kScale});
    }
    for (std::size_t i = 0; i < contour.points.size(); ++i) {
      const Vec2& a = contour.points[i];
      const Vec2& b = contour.points[(i + 1) % contour.points.size()];
      contour.signed_area += a.x * b.y - b.x * a.y;
    }
    contour.signed_area *= 0.5;
    contour.hole = contour.signed_area < 0.0;
    contours.push_back(std::move(contour));
  }
  return {std::move(contours), ""};
}

PolygonOperationResult offset_polygons(const std::vector<Contour>& contours,
                                       double delta) {
  Clipper2Lib::Paths64 paths;
  if (!to_paths(contours, paths)) return {{}, "Polygon coordinate is non-finite or exceeds the integer range"};
  try {
    return from_closed_paths(Clipper2Lib::InflatePaths(
        paths, delta * kScale, Clipper2Lib::JoinType::Round,
        Clipper2Lib::EndType::Polygon));
  } catch (const Clipper2Lib::Clipper2Exception& exception) {
    return {{}, exception.what()};
  }
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

std::vector<Contour> remove_small_contours(
    const std::vector<Contour>& contours, double minimum_area) {
  if (!std::isfinite(minimum_area) || minimum_area <= 0.0) return contours;
  std::vector<Contour> filtered;
  for (const Contour& contour : contours) {
    if (std::abs(contour.signed_area) >= minimum_area) {
      filtered.push_back(contour);
    }
  }
  return filtered;
}

ManufacturingCleanupResult apply_manufacturing_cleanup(
    const std::vector<Contour>& contours,
    const ManufacturingCleanupOptions& options) {
  ManufacturingCleanupResult result;
  result.contours = contours;
  const double thresholds[] = {options.minimum_feature_width,
                               options.minimum_island_area,
                               options.minimum_hole_width,
                               options.minimum_bridge_width};
  for (double threshold : thresholds) {
    if (!std::isfinite(threshold) || threshold < 0.0) {
      result.ok = false;
      result.error = "Manufacturing cleanup thresholds must be finite and non-negative";
      return result;
    }
  }
  if (options.mode == CleanupMode::PRESERVE) return result;

  for (const Contour& contour : contours) {
    const double width = estimated_width(contour);
    if (options.minimum_feature_width > 0.0 && width < options.minimum_feature_width)
      result.warnings.push_back("Contour " + std::to_string(&contour - contours.data()) +
                                " is narrower than the minimum feature width");
    if (contour.hole && options.minimum_hole_width > 0.0 && width < options.minimum_hole_width)
      result.warnings.push_back("Hole " + std::to_string(&contour - contours.data()) +
                                " is narrower than the minimum hole width");
    if (!contour.hole && options.minimum_island_area > 0.0 &&
        std::abs(contour.signed_area) < options.minimum_island_area)
      result.warnings.push_back("Island " + std::to_string(&contour - contours.data()) +
                                " is smaller than the minimum island area");
    if (!contour.hole && options.minimum_bridge_width > 0.0 &&
        width < options.minimum_bridge_width)
      result.warnings.push_back("Contour " + std::to_string(&contour - contours.data()) +
                                " may contain a bridge narrower than the minimum bridge width");
  }
  if (options.mode == CleanupMode::WARN) return result;

  const double opening_width = std::max(options.minimum_feature_width,
                                        options.minimum_bridge_width);
  if (opening_width > 0.0) {
    const auto eroded = offset_polygons(result.contours, -opening_width / 2.0);
    if (!eroded.ok()) { result.ok = false; result.error = eroded.error; return result; }
    const auto opened = offset_polygons(eroded.contours, opening_width / 2.0);
    if (!opened.ok()) { result.ok = false; result.error = opened.error; return result; }
    result.contours = opened.contours;
  }
  if (options.minimum_hole_width > 0.0) {
    const auto expanded = offset_polygons(result.contours, options.minimum_hole_width / 2.0);
    if (!expanded.ok()) { result.ok = false; result.error = expanded.error; return result; }
    const auto closed = offset_polygons(expanded.contours, -options.minimum_hole_width / 2.0);
    if (!closed.ok()) { result.ok = false; result.error = closed.error; return result; }
    result.contours = closed.contours;
  }
  if (options.minimum_island_area > 0.0) {
    std::vector<Contour> retained;
    for (const Contour& contour : result.contours) {
      if (contour.hole || std::abs(contour.signed_area) >= options.minimum_island_area)
        retained.push_back(contour);
    }
    result.contours = std::move(retained);
  }
  result.changed = result.contours.size() != contours.size();
  return result;
}

}  // namespace layer_cut
