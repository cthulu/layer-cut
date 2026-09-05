#include "stacked_preview.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>

namespace layer_cut {
namespace {

constexpr double kEpsilon = 1e-9;

double cross(const Vec2& a, const Vec2& b, const Vec2& c) {
  return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

double area(const std::vector<Vec2>& points) {
  double value = 0.0;
  for (std::size_t i = 0; i < points.size(); ++i) {
    const Vec2& a = points[i];
    const Vec2& b = points[(i + 1) % points.size()];
    value += a.x * b.y - b.x * a.y;
  }
  return value * 0.5;
}

bool finite(const Vec2& point) {
  return std::isfinite(point.x) && std::isfinite(point.y);
}

bool on_segment(const Vec2& a, const Vec2& b, const Vec2& point) {
  return std::abs(cross(a, b, point)) <= kEpsilon &&
         point.x >= std::min(a.x, b.x) - kEpsilon &&
         point.x <= std::max(a.x, b.x) + kEpsilon &&
         point.y >= std::min(a.y, b.y) - kEpsilon &&
         point.y <= std::max(a.y, b.y) + kEpsilon;
}

int orientation(const Vec2& a, const Vec2& b, const Vec2& c) {
  const double value = cross(a, b, c);
  if (std::abs(value) <= kEpsilon) return 0;
  return value > 0.0 ? 1 : -1;
}

bool segments_intersect(const Vec2& a, const Vec2& b, const Vec2& c,
                        const Vec2& d) {
  const int ab_c = orientation(a, b, c);
  const int ab_d = orientation(a, b, d);
  const int cd_a = orientation(c, d, a);
  const int cd_b = orientation(c, d, b);
  if (ab_c != ab_d && cd_a != cd_b) return true;
  return (ab_c == 0 && on_segment(a, b, c)) ||
         (ab_d == 0 && on_segment(a, b, d)) ||
         (cd_a == 0 && on_segment(c, d, a)) ||
         (cd_b == 0 && on_segment(c, d, b));
}

bool contains_point(const std::vector<Vec2>& polygon, const Vec2& point) {
  bool inside = false;
  for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size();
       j = i++) {
    const Vec2& a = polygon[i];
    const Vec2& b = polygon[j];
    if (on_segment(a, b, point)) return true;
    const bool crosses = ((a.y > point.y) != (b.y > point.y)) &&
                         (point.x < (b.x - a.x) * (point.y - a.y) /
                                        (b.y - a.y) + a.x);
    if (crosses) inside = !inside;
  }
  return inside;
}

void remove_duplicate_points(std::vector<Vec2>& points) {
  std::vector<Vec2> filtered;
  for (const Vec2& point : points) {
    if (filtered.empty() || std::abs(filtered.back().x - point.x) > kEpsilon ||
        std::abs(filtered.back().y - point.y) > kEpsilon) {
      filtered.push_back(point);
    }
  }
  if (filtered.size() > 1 &&
      std::abs(filtered.front().x - filtered.back().x) <= kEpsilon &&
      std::abs(filtered.front().y - filtered.back().y) <= kEpsilon) {
    filtered.pop_back();
  }
  points = std::move(filtered);
}

bool bridge_is_clear(const std::vector<Vec2>& outer,
                     const std::vector<Vec2>& hole, std::size_t outer_index,
                     std::size_t hole_index) {
  const Vec2& a = outer[outer_index];
  const Vec2& b = hole[hole_index];
  for (std::size_t i = 0; i < outer.size(); ++i) {
    const std::size_t next = (i + 1) % outer.size();
    if (i == outer_index || next == outer_index) continue;
    if (segments_intersect(a, b, outer[i], outer[next])) return false;
  }
  for (std::size_t i = 0; i < hole.size(); ++i) {
    const std::size_t next = (i + 1) % hole.size();
    if (i == hole_index || next == hole_index) continue;
    if (segments_intersect(a, b, hole[i], hole[next])) return false;
  }
  const Vec2 midpoint{(a.x + b.x) * 0.5, (a.y + b.y) * 0.5};
  return contains_point(outer, midpoint);
}

bool bridge_hole(std::vector<Vec2>& polygon, const std::vector<Vec2>& hole,
                 std::string* error) {
  if (hole.size() < 3) return true;
  std::size_t hole_index = 0;
  for (std::size_t i = 1; i < hole.size(); ++i) {
    if (hole[i].x > hole[hole_index].x ||
        (hole[i].x == hole[hole_index].x && hole[i].y < hole[hole_index].y)) {
      hole_index = i;
    }
  }

  std::vector<std::size_t> candidates(polygon.size());
  std::iota(candidates.begin(), candidates.end(), 0);
  std::sort(candidates.begin(), candidates.end(), [&](std::size_t a,
                                                      std::size_t b) {
    const double da = (polygon[a].x - hole[hole_index].x) *
                          (polygon[a].x - hole[hole_index].x) +
                      (polygon[a].y - hole[hole_index].y) *
                          (polygon[a].y - hole[hole_index].y);
    const double db = (polygon[b].x - hole[hole_index].x) *
                          (polygon[b].x - hole[hole_index].x) +
                      (polygon[b].y - hole[hole_index].y) *
                          (polygon[b].y - hole[hole_index].y);
    return da < db;
  });

  std::size_t outer_index = polygon.size();
  for (const std::size_t candidate : candidates) {
    if (bridge_is_clear(polygon, hole, candidate, hole_index)) {
      outer_index = candidate;
      break;
    }
  }
  if (outer_index == polygon.size()) {
    if (error) *error = "Unable to bridge a hole for preview triangulation";
    return false;
  }

  std::vector<Vec2> merged;
  merged.reserve(polygon.size() + hole.size() + 2);
  for (std::size_t i = 0; i <= outer_index; ++i) merged.push_back(polygon[i]);
  merged.push_back(hole[hole_index]);
  // Holes are clockwise; traversing in their existing order preserves the
  // orientation of the bridged polygon.
  for (std::size_t step = 1; step < hole.size(); ++step) {
    merged.push_back(hole[(hole_index + step) % hole.size()]);
  }
  merged.push_back(hole[hole_index]);
  merged.push_back(polygon[outer_index]);
  for (std::size_t i = outer_index + 1; i < polygon.size(); ++i) {
    merged.push_back(polygon[i]);
  }
  remove_duplicate_points(merged);
  polygon = std::move(merged);
  return true;
}

bool point_in_triangle(const Vec2& point, const Vec2& a, const Vec2& b,
                       const Vec2& c) {
  const double first = cross(a, b, point);
  const double second = cross(b, c, point);
  const double third = cross(c, a, point);
  // Points on a bridge edge, including duplicated bridge endpoints, do not
  // block an ear. Only strictly interior points invalidate the candidate.
  return first > kEpsilon && second > kEpsilon && third > kEpsilon;
}

bool triangulate_simple(std::vector<Vec2> polygon,
                        std::vector<std::array<Vec2, 3>>& triangles,
                        std::string* error) {
  remove_duplicate_points(polygon);
  if (polygon.size() < 3 || std::abs(area(polygon)) <= kEpsilon) return true;
  if (area(polygon) < 0.0) std::reverse(polygon.begin(), polygon.end());

  std::vector<std::size_t> remaining(polygon.size());
  std::iota(remaining.begin(), remaining.end(), 0);
  const std::size_t maximum_iterations = polygon.size() * polygon.size();
  std::size_t iterations = 0;
  while (remaining.size() > 2) {
    bool clipped = false;
    for (std::size_t position = 0; position < remaining.size(); ++position) {
      const std::size_t previous =
          remaining[(position + remaining.size() - 1) % remaining.size()];
      const std::size_t current = remaining[position];
      const std::size_t next = remaining[(position + 1) % remaining.size()];
      if (cross(polygon[previous], polygon[current], polygon[next]) <=
          kEpsilon) {
        continue;
      }
      bool contains_vertex = false;
      for (const std::size_t candidate : remaining) {
        if (candidate == previous || candidate == current || candidate == next)
          continue;
        if (point_in_triangle(polygon[candidate], polygon[previous],
                              polygon[current], polygon[next])) {
          contains_vertex = true;
          break;
        }
      }
      if (contains_vertex) continue;
      triangles.push_back(
          {polygon[previous], polygon[current], polygon[next]});
      remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(position));
      clipped = true;
      break;
    }
    if (!clipped || ++iterations > maximum_iterations) {
      if (error) *error = "Unable to triangulate a preview contour";
      return false;
    }
  }
  return true;
}

bool triangulate_contours(const std::vector<Contour>& contours,
                          std::vector<std::array<Vec2, 3>>& triangles,
                          std::string* error) {
  std::vector<std::size_t> outer_indices;
  for (std::size_t i = 0; i < contours.size(); ++i) {
    if (!contours[i].hole) outer_indices.push_back(i);
  }
  for (const std::size_t outer_index : outer_indices) {
    std::vector<Vec2> polygon = contours[outer_index].points;
    std::vector<std::size_t> holes;
    for (std::size_t i = 0; i < contours.size(); ++i) {
      if (contours[i].hole && !contours[i].points.empty() &&
          contains_point(polygon, contours[i].points.front())) {
        holes.push_back(i);
      }
    }
    for (const std::size_t hole_index : holes) {
      if (!bridge_hole(polygon, contours[hole_index].points, error)) return false;
    }
    if (!triangulate_simple(std::move(polygon), triangles, error)) return false;
  }
  return true;
}

void add_triangle(std::vector<Triangle>& triangles, const Vec3& a,
                  const Vec3& b, const Vec3& c) {
  triangles.push_back({{0.0f, 0.0f, 0.0f}, a, b, c});
}

void add_cap(std::vector<Triangle>& output,
             const std::vector<std::array<Vec2, 3>>& triangles, double z,
             bool top) {
  for (const auto& triangle : triangles) {
    const Vec3 a{static_cast<float>(triangle[0].x),
                 static_cast<float>(triangle[0].y), static_cast<float>(z)};
    const Vec3 b{static_cast<float>(triangle[1].x),
                 static_cast<float>(triangle[1].y), static_cast<float>(z)};
    const Vec3 c{static_cast<float>(triangle[2].x),
                 static_cast<float>(triangle[2].y), static_cast<float>(z)};
    if (top) {
      add_triangle(output, a, b, c);
    } else {
      add_triangle(output, a, c, b);
    }
  }
}

void add_walls(std::vector<Triangle>& output, const std::vector<Contour>& contours,
               double bottom_z, double top_z) {
  for (const Contour& contour : contours) {
    if (contour.points.size() < 3) continue;
    for (std::size_t i = 0; i < contour.points.size(); ++i) {
      const Vec2& current = contour.points[i];
      const Vec2& next = contour.points[(i + 1) % contour.points.size()];
      const Vec3 bottom_a{static_cast<float>(current.x),
                          static_cast<float>(current.y),
                          static_cast<float>(bottom_z)};
      const Vec3 bottom_b{static_cast<float>(next.x),
                          static_cast<float>(next.y),
                          static_cast<float>(bottom_z)};
      const Vec3 top_a{static_cast<float>(current.x),
                       static_cast<float>(current.y), static_cast<float>(top_z)};
      const Vec3 top_b{static_cast<float>(next.x),
                       static_cast<float>(next.y), static_cast<float>(top_z)};
      if (!contour.hole) {
        add_triangle(output, bottom_a, bottom_b, top_b);
        add_triangle(output, bottom_a, top_b, top_a);
      } else {
        add_triangle(output, bottom_a, top_a, top_b);
        add_triangle(output, bottom_a, top_b, bottom_b);
      }
    }
  }
}

}  // namespace

StackedPreviewResult make_stacked_preview(
    const std::vector<SliceLayer>& layers, double layer_height) {
  StackedPreviewResult result;
  if (!std::isfinite(layer_height) || layer_height <= 0.0) {
    result.error = "Preview layer height must be positive and finite";
    return result;
  }

  for (const SliceLayer& layer : layers) {
    if (layer.contours.empty()) continue;
    for (const Contour& contour : layer.contours) {
      for (const Vec2& point : contour.points) {
        if (!finite(point)) {
          result.error = "Preview contour contains a non-finite coordinate";
          return result;
        }
      }
    }

    std::vector<std::array<Vec2, 3>> cap_triangles;
    std::string triangulation_error;
    if (!triangulate_contours(layer.contours, cap_triangles,
                              &triangulation_error)) {
      result.error = "Layer " + std::to_string(layer.index) + ": " +
                     triangulation_error;
      return result;
    }
    const double bottom_z = layer.z - layer_height * 0.5;
    const double top_z = layer.z + layer_height * 0.5;
    add_cap(result.triangles, cap_triangles, bottom_z, false);
    add_cap(result.triangles, cap_triangles, top_z, true);
    add_walls(result.triangles, layer.contours, bottom_z, top_z);
  }
  if (result.triangles.empty()) result.error = "Preview contains no non-empty layers";
  return result;
}

}  // namespace layer_cut
