#include "plane_intersection.h"

#include <cmath>

namespace layer_cut {

namespace {

struct PointWithEdges {
  Vec2 point;
  uint8_t edge_mask;
};

double distance_squared(const Vec2& a, const Vec2& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  return dx * dx + dy * dy;
}

bool same_point(const Vec2& a, const Vec2& b, double epsilon) {
  return distance_squared(a, b) <= epsilon * epsilon;
}

void add_point(std::vector<PointWithEdges>& points, Vec2 point,
               uint8_t edge_mask, double epsilon) {
  for (PointWithEdges& existing : points) {
    if (same_point(existing.point, point, epsilon)) {
      existing.edge_mask |= edge_mask;
      return;
    }
  }
  points.push_back({point, edge_mask});
}

void add_segment(std::vector<PlaneSegment>& segments, Vec2 a, Vec2 b,
                 uint8_t edge_mask, double epsilon) {
  if (same_point(a, b, epsilon)) return;
  for (PlaneSegment& existing : segments) {
    const bool same_direction = same_point(existing.a, a, epsilon) &&
                                same_point(existing.b, b, epsilon);
    const bool reverse_direction = same_point(existing.a, b, epsilon) &&
                                   same_point(existing.b, a, epsilon);
    if (same_direction || reverse_direction) {
      existing.source_edge_mask |= edge_mask;
      return;
    }
  }
  segments.push_back({a, b, edge_mask});
}

}  // namespace

std::vector<PlaneSegment> intersect_triangle_with_plane(
    const Triangle& triangle, double plane_z,
    const PlaneIntersectionOptions& options) {
  std::vector<PlaneSegment> segments;
  if (!std::isfinite(plane_z) || !std::isfinite(options.epsilon) ||
      options.epsilon < 0.0) {
    return segments;
  }

  const Vec3 vertices[] = {triangle.a, triangle.b, triangle.c};
  const double distances[] = {
      static_cast<double>(vertices[0].z) - plane_z,
      static_cast<double>(vertices[1].z) - plane_z,
      static_cast<double>(vertices[2].z) - plane_z};
  const uint8_t edge_masks[] = {1, 2, 4};
  const int edge_start[] = {0, 1, 2};
  const int edge_end[] = {1, 2, 0};

  int on_plane_count = 0;
  for (double distance : distances) {
    if (std::abs(distance) <= options.epsilon) ++on_plane_count;
  }
  if (on_plane_count == 3) {
    for (int edge = 0; edge < 3; ++edge) {
      add_segment(segments,
                  {vertices[edge_start[edge]].x, vertices[edge_start[edge]].y},
                  {vertices[edge_end[edge]].x, vertices[edge_end[edge]].y},
                  edge_masks[edge], options.epsilon);
    }
    return segments;
  }

  std::vector<PointWithEdges> points;
  for (int edge = 0; edge < 3; ++edge) {
    const int start = edge_start[edge];
    const int end = edge_end[edge];
    const double start_distance = distances[start];
    const double end_distance = distances[end];
    const Vec2 start_point = {vertices[start].x, vertices[start].y};
    const Vec2 end_point = {vertices[end].x, vertices[end].y};
    const bool start_on = std::abs(start_distance) <= options.epsilon;
    const bool end_on = std::abs(end_distance) <= options.epsilon;

    if (start_on && end_on) {
      add_segment(segments, start_point, end_point, edge_masks[edge],
                  options.epsilon);
    } else if (start_on) {
      add_point(points, start_point, edge_masks[edge], options.epsilon);
    } else if (end_on) {
      add_point(points, end_point, edge_masks[edge], options.epsilon);
    } else if ((start_distance < 0.0) != (end_distance < 0.0)) {
      const double fraction = start_distance / (start_distance - end_distance);
      add_point(points,
                {start_point.x + fraction * (end_point.x - start_point.x),
                 start_point.y + fraction * (end_point.y - start_point.y)},
                edge_masks[edge], options.epsilon);
    }
  }

  if (segments.empty() && points.size() == 2) {
    add_segment(segments, points[0].point, points[1].point,
                points[0].edge_mask | points[1].edge_mask, options.epsilon);
  }
  return segments;
}

}  // namespace layer_cut
