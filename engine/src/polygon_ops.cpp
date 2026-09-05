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

bool contains_point(const std::vector<Vec2>& polygon, const Vec2& point) {
  bool inside = false;
  for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size();
       j = i++) {
    const Vec2& a = polygon[i];
    const Vec2& b = polygon[j];
    const bool crosses = ((a.y > point.y) != (b.y > point.y)) &&
                         (point.x < (b.x - a.x) * (point.y - a.y) /
                                        (b.y - a.y) + a.x);
    if (crosses) inside = !inside;
  }
  return inside;
}

void normalize_contour(Contour& contour) {
  double area = contour.signed_area;
  if ((contour.hole && area > 0.0) || (!contour.hole && area < 0.0)) {
    std::reverse(contour.points.begin(), contour.points.end());
    contour.signed_area = -area;
  }
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
    contours.push_back(std::move(contour));
  }

  // Clipper offset output orientation is not a reliable hole contract for
  // polygons with touching or newly split regions. Reconstruct nesting depth
  // from containment, then normalize the engine's outer=CCW/hole=CW winding.
  for (std::size_t i = 0; i < contours.size(); ++i) {
    std::size_t nesting_depth = 0;
    for (std::size_t j = 0; j < contours.size(); ++j) {
      if (i != j && contains_point(contours[j].points, contours[i].points[0])) {
        ++nesting_depth;
      }
    }
    contours[i].hole = (nesting_depth % 2) == 1;
    normalize_contour(contours[i]);
  }
  return {std::move(contours), ""};
}

double filled_area(const std::vector<Contour>& contours) {
  double area = 0.0;
  for (const Contour& contour : contours) {
    area += contour.hole ? -std::abs(contour.signed_area)
                         : std::abs(contour.signed_area);
  }
  return area;
}

bool filled_area_increased(const std::vector<Contour>& first,
                           const std::vector<Contour>& second) {
  return filled_area(first) > filled_area(second) + 1e-6;
}

bool filled_area_decreased(const std::vector<Contour>& first,
                           const std::vector<Contour>& second) {
  return filled_area(first) < filled_area(second) - 1e-6;
}

bool nearly_equal_contour(const Contour& first, const Contour& second) {
  if (first.hole != second.hole || first.points.size() != second.points.size() ||
      std::abs(first.signed_area - second.signed_area) > 1e-7) return false;
  const double scale = std::max(1.0, std::sqrt(std::abs(first.signed_area)));
  const double tolerance = 1e-6 * scale;
  for (std::size_t i = 0; i < first.points.size(); ++i) {
    if (std::abs(first.points[i].x - second.points[i].x) > tolerance ||
        std::abs(first.points[i].y - second.points[i].y) > tolerance) return false;
  }
  return true;
}

bool contours_equal(const std::vector<Contour>& first,
                    const std::vector<Contour>& second) {
  if (first.size() != second.size()) return false;
  for (std::size_t i = 0; i < first.size(); ++i) {
    if (!nearly_equal_contour(first[i], second[i])) return false;
  }
  return true;
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

  if (options.mode == CleanupMode::WARN) {
    const auto probe = options;
    if (probe.minimum_feature_width > 0.0 || probe.minimum_bridge_width > 0.0) {
      const double width = std::max(probe.minimum_feature_width,
                                    probe.minimum_bridge_width);
      const auto eroded = offset_polygons(contours, -width / 2.0);
      if (!eroded.ok()) { result.ok = false; result.error = eroded.error; return result; }
      const auto opened = offset_polygons(eroded.contours, width / 2.0);
      if (!opened.ok()) { result.ok = false; result.error = opened.error; return result; }
      if (filled_area_decreased(opened.contours, contours)) {
        for (std::size_t i = 0; i < contours.size(); ++i) {
          result.warnings.push_back("Contour " + std::to_string(i) +
                                    " contains a feature narrower than the minimum width");
        }
      }
    }
    if (probe.minimum_hole_width > 0.0) {
      const auto expanded = offset_polygons(contours, probe.minimum_hole_width / 2.0);
      if (!expanded.ok()) { result.ok = false; result.error = expanded.error; return result; }
      const auto closed = offset_polygons(expanded.contours,
                                          -probe.minimum_hole_width / 2.0);
      if (!closed.ok()) { result.ok = false; result.error = closed.error; return result; }
      if (filled_area_increased(closed.contours, contours)) {
        for (std::size_t i = 0; i < contours.size(); ++i) {
          if (contours[i].hole)
            result.warnings.push_back("Hole " + std::to_string(i) +
                                      " is narrower than the minimum hole width");
        }
      }
    }
    for (std::size_t i = 0; i < contours.size(); ++i) {
      const Contour& contour = contours[i];
      if (!contour.hole && options.minimum_island_area > 0.0 &&
          std::abs(contour.signed_area) < options.minimum_island_area)
        result.warnings.push_back("Island " + std::to_string(i) +
                                  " is smaller than the minimum island area");
    }
    result.contours = contours;
    result.changed = false;
    return result;
  }

  const auto apply_opening = [&]() -> bool {
    const double width = std::max(options.minimum_feature_width,
                                  options.minimum_bridge_width);
    if (width <= 0.0) return true;
    const auto eroded = offset_polygons(result.contours, -width / 2.0);
    if (!eroded.ok()) { result.error = eroded.error; return false; }
    const auto opened = offset_polygons(eroded.contours, width / 2.0);
    if (!opened.ok()) { result.error = opened.error; return false; }
    result.contours = opened.contours;
    return true;
  };
  const auto apply_closing = [&]() -> bool {


    if (options.minimum_hole_width <= 0.0) return true;
    const auto expanded = offset_polygons(result.contours,
                                          options.minimum_hole_width / 2.0);
    if (!expanded.ok()) { result.error = expanded.error; return false; }
    const auto closed = offset_polygons(expanded.contours,
                                        -options.minimum_hole_width / 2.0);
    if (!closed.ok()) { result.error = closed.error; return false; }
    result.contours = closed.contours;
    return true;
  };
  const auto filter_islands = [&]() {
    if (options.minimum_island_area <= 0.0) return;
    std::vector<Contour> retained;
    for (const Contour& contour : result.contours) {
      if (contour.hole || std::abs(contour.signed_area) >= options.minimum_island_area)
        retained.push_back(contour);
    }
    result.contours = std::move(retained);
  };

  std::vector<Contour> opened = result.contours;
  if (options.minimum_feature_width > 0.0 || options.minimum_bridge_width > 0.0) {
    if (!apply_opening()) { result.ok = false; return result; }
    opened = result.contours;
  }
  result.contours = opened;
  if (options.minimum_hole_width > 0.0) {
    if (!apply_closing()) { result.ok = false; return result; }
  }
  std::vector<Contour> closed = result.contours;
  filter_islands();
  const std::vector<Contour> filtered = result.contours;

  const bool opening_changed = filled_area_decreased(opened, contours);
  const bool closing_changed = filled_area_increased(closed, opened);
  for (std::size_t i = 0; i < contours.size(); ++i) {
    const Contour& contour = contours[i];
    const bool small_island = !contour.hole && options.minimum_island_area > 0.0 &&
                              std::abs(contour.signed_area) < options.minimum_island_area;
    if (options.minimum_feature_width > 0.0 && opening_changed)
      result.warnings.push_back("Contour " + std::to_string(i) +
                                " contains a feature narrower than the minimum width");
    if (options.minimum_bridge_width > 0.0 && opening_changed)
      result.warnings.push_back("Contour " + std::to_string(i) +
                                " contains a bridge narrower than the minimum width");
    if (options.minimum_hole_width > 0.0 && contour.hole && closing_changed)
      result.warnings.push_back("Hole " + std::to_string(i) +
                                " is narrower than the minimum hole width");
    if (small_island)
      result.warnings.push_back("Island " + std::to_string(i) +
                                " is smaller than the minimum island area");
  }
  result.contours = filtered;
  result.changed = !contours_equal(result.contours, contours);
  // WARN mode: re-assign original contours so geometry is unmodified,
  // but warnings still reference indices in the original contours.
  if (options.mode == CleanupMode::WARN) result.contours = contours;
  return result;
}

}  // namespace layer_cut
