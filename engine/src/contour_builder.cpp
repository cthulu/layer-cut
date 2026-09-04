#include "contour_builder.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

namespace layer_cut {

namespace {

struct CellKey {
  int64_t x, y;

  bool operator==(const CellKey& other) const {
    return x == other.x && y == other.y;
  }
};

struct CellKeyHash {
  std::size_t operator()(const CellKey& key) const {
    return std::hash<int64_t>{}(key.x) ^ (std::hash<int64_t>{}(key.y) << 1);
  }
};

struct EdgeKey {
  std::size_t first, second;

  bool operator==(const EdgeKey& other) const {
    return first == other.first && second == other.second;
  }
};

struct EdgeKeyHash {
  std::size_t operator()(const EdgeKey& key) const {
    return key.first ^ (key.second << 1);
  }
};

struct Edge {
  std::size_t first, second;
};

double squared_distance(const Vec2& a, const Vec2& b) {
  const double dx = a.x - b.x;
  const double dy = a.y - b.y;
  return dx * dx + dy * dy;
}

double signed_area(const std::vector<Vec2>& points) {
  double area = 0.0;
  for (std::size_t i = 0; i < points.size(); ++i) {
    const Vec2& current = points[i];
    const Vec2& next = points[(i + 1) % points.size()];
    area += current.x * next.y - next.x * current.y;
  }
  return area / 2.0;
}

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

}  // namespace

ContourBuildResult build_contours(const std::vector<PlaneSegment>& segments,
                                   const ContourBuildOptions& options) {
  ContourBuildResult result;
  if (!std::isfinite(options.snap_tolerance) ||
      options.snap_tolerance <= 0.0) {
    return result;
  }

  const double tolerance_squared = options.snap_tolerance * options.snap_tolerance;
  std::vector<Vec2> nodes;
  std::unordered_map<CellKey, std::vector<std::size_t>, CellKeyHash> cells;

  const auto snap = [&](Vec2 point) -> std::size_t {
    const int64_t cell_x = static_cast<int64_t>(std::floor(
        point.x / options.snap_tolerance));
    const int64_t cell_y = static_cast<int64_t>(std::floor(
        point.y / options.snap_tolerance));
    for (int dx = -1; dx <= 1; ++dx) {
      for (int dy = -1; dy <= 1; ++dy) {
        const CellKey key = {cell_x + dx, cell_y + dy};
        const auto found = cells.find(key);
        if (found == cells.end()) continue;
        for (std::size_t node : found->second) {
          if (squared_distance(nodes[node], point) <= tolerance_squared) {
            return node;
          }
        }
      }
    }
    const std::size_t node = nodes.size();
    nodes.push_back(point);
    cells[{cell_x, cell_y}].push_back(node);
    return node;
  };

  std::vector<Edge> edges;
  std::unordered_set<EdgeKey, EdgeKeyHash> unique_edges;
  for (const PlaneSegment& segment : segments) {
    const std::size_t first = snap(segment.a);
    const std::size_t second = snap(segment.b);
    if (first == second) continue;
    const EdgeKey key = first < second ? EdgeKey{first, second}
                                      : EdgeKey{second, first};
    if (unique_edges.insert(key).second) edges.push_back({first, second});
  }

  std::vector<std::vector<std::size_t>> adjacency(nodes.size());
  for (std::size_t edge = 0; edge < edges.size(); ++edge) {
    adjacency[edges[edge].first].push_back(edge);
    adjacency[edges[edge].second].push_back(edge);
  }
  for (const auto& incident : adjacency) {
    if (incident.size() == 1) {
      result.diagnostics.push_back(
          {ContourDiagnosticCategory::DANGLING_EDGE, 1});
    } else if (incident.size() > 2) {
      result.diagnostics.push_back(
          {ContourDiagnosticCategory::BRANCHING_VERTEX, incident.size()});
    }
  }

  std::vector<bool> used(edges.size(), false);
  for (std::size_t initial_edge = 0; initial_edge < edges.size();
       ++initial_edge) {
    if (used[initial_edge]) continue;
    const std::size_t start = edges[initial_edge].first;
    std::size_t current = start;
    std::size_t edge = initial_edge;
    std::vector<Vec2> points;
    bool closed = false;
    while (true) {
      used[edge] = true;
      const Edge& current_edge = edges[edge];
      const std::size_t next = current_edge.first == current
                                   ? current_edge.second
                                   : current_edge.first;
      points.push_back(nodes[current]);
      if (next == start) {
        closed = true;
        break;
      }
      const auto& incident = adjacency[next];
      std::size_t next_edge = edges.size();
      for (std::size_t candidate : incident) {
        if (!used[candidate]) {
          next_edge = candidate;
          break;
        }
      }
      if (next_edge == edges.size()) break;
      current = next;
      edge = next_edge;
    }
    if (!closed || points.size() < 3) {
      result.diagnostics.push_back(
          {ContourDiagnosticCategory::OPEN_CONTOUR, points.size()});
      continue;
    }
    result.contours.push_back({std::move(points), false, 0.0});
  }

  for (std::size_t i = 0; i < result.contours.size(); ++i) {
    std::size_t nesting_depth = 0;
    for (std::size_t j = 0; j < result.contours.size(); ++j) {
      if (i != j && contains_point(result.contours[j].points,
                                   result.contours[i].points[0])) {
        ++nesting_depth;
      }
    }
    Contour& contour = result.contours[i];
    contour.hole = (nesting_depth % 2) == 1;
    double area = signed_area(contour.points);
    const bool should_reverse = contour.hole ? area > 0.0 : area < 0.0;
    if (should_reverse) {
      std::reverse(contour.points.begin(), contour.points.end());
      area = -area;
    }
    contour.signed_area = area;
  }
  return result;
}

}  // namespace layer_cut
