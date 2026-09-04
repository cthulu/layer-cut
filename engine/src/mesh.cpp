#include "mesh.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace layer_cut {

namespace {

constexpr double kAreaEpsilon = 1e-12;
constexpr double kVertexEpsilon = 1e-6;

struct VertexKey {
  int64_t x, y, z;

  bool operator==(const VertexKey& other) const {
    return x == other.x && y == other.y && z == other.z;
  }
};

struct VertexKeyHash {
  std::size_t operator()(const VertexKey& key) const {
    std::size_t hash = static_cast<std::size_t>(key.x);
    hash ^= static_cast<std::size_t>(key.y) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    hash ^= static_cast<std::size_t>(key.z) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    return hash;
  }
};

struct EdgeKey {
  VertexKey first, second;

  bool operator==(const EdgeKey& other) const {
    return first == other.first && second == other.second;
  }
};

struct EdgeKeyHash {
  std::size_t operator()(const EdgeKey& key) const {
    VertexKeyHash vertex_hash;
    return vertex_hash(key.first) ^ (vertex_hash(key.second) << 1);
  }
};

VertexKey vertex_key(const Vec3& vertex) {
  const auto quantize = [](float coordinate) {
    const double scaled = static_cast<double>(coordinate) / kVertexEpsilon;
    const double max_value =
        static_cast<double>(std::numeric_limits<int64_t>::max());
    const double min_value =
        static_cast<double>(std::numeric_limits<int64_t>::min());
    if (scaled >= max_value) return std::numeric_limits<int64_t>::max();
    if (scaled <= min_value) return std::numeric_limits<int64_t>::min();
    return static_cast<int64_t>(std::llround(scaled));
  };
  return {quantize(vertex.x), quantize(vertex.y), quantize(vertex.z)};
}

EdgeKey edge_key(const VertexKey& a, const VertexKey& b) {
  const bool a_first =
      a.x < b.x || (a.x == b.x && (a.y < b.y ||
                                   (a.y == b.y && a.z <= b.z)));
  return a_first ? EdgeKey{a, b} : EdgeKey{b, a};
}

Vec3 subtract(const Vec3& a, const Vec3& b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 cross(const Vec3& a, const Vec3& b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
          a.x * b.y - a.y * b.x};
}

double dot(const Vec3& a, const Vec3& b) {
  return static_cast<double>(a.x) * b.x + static_cast<double>(a.y) * b.y +
         static_cast<double>(a.z) * b.z;
}

bool finite(const Vec3& vertex) {
  return std::isfinite(vertex.x) && std::isfinite(vertex.y) &&
         std::isfinite(vertex.z);
}

void add_diagnostic(ValidationResult& result, DiagnosticSeverity severity,
                    DiagnosticCategory category, std::size_t triangle_index,
                    std::size_t affected_count, std::string message) {
  result.diagnostics.push_back(
      {severity, category, triangle_index, affected_count, std::move(message)});
}

bool overlaps(const Vec3& a_min, const Vec3& a_max, const Vec3& b_min,
              const Vec3& b_max) {
  return a_min.x <= b_max.x && b_min.x <= a_max.x &&
         a_min.y <= b_max.y && b_min.y <= a_max.y &&
         a_min.z <= b_max.z && b_min.z <= a_max.z;
}

bool segment_hits_triangle(const Vec3& start, const Vec3& end,
                           const Triangle& triangle) {
  // Moller-Trumbore intersection, restricted to the finite segment.
  const Vec3 direction = subtract(end, start);
  const Vec3 edge1 = subtract(triangle.b, triangle.a);
  const Vec3 edge2 = subtract(triangle.c, triangle.a);
  const Vec3 p = cross(direction, edge2);
  const double determinant = dot(edge1, p);
  if (std::abs(determinant) <= kAreaEpsilon) return false;

  const double inverse = 1.0 / determinant;
  const Vec3 to_start = subtract(start, triangle.a);
  const double u = dot(to_start, p) * inverse;
  if (u < -kVertexEpsilon || u > 1.0 + kVertexEpsilon) return false;
  const Vec3 q = cross(to_start, edge1);
  const double v = dot(direction, q) * inverse;
  if (v < -kVertexEpsilon || u + v > 1.0 + kVertexEpsilon) return false;
  const double distance = dot(edge2, q) * inverse;
  return distance >= -kVertexEpsilon && distance <= 1.0 + kVertexEpsilon;
}

bool triangles_intersect(const Triangle& first, const Triangle& second) {
  const Vec3 first_min = {
      std::min({first.a.x, first.b.x, first.c.x}),
      std::min({first.a.y, first.b.y, first.c.y}),
      std::min({first.a.z, first.b.z, first.c.z})};
  const Vec3 first_max = {
      std::max({first.a.x, first.b.x, first.c.x}),
      std::max({first.a.y, first.b.y, first.c.y}),
      std::max({first.a.z, first.b.z, first.c.z})};
  const Vec3 second_min = {
      std::min({second.a.x, second.b.x, second.c.x}),
      std::min({second.a.y, second.b.y, second.c.y}),
      std::min({second.a.z, second.b.z, second.c.z})};
  const Vec3 second_max = {
      std::max({second.a.x, second.b.x, second.c.x}),
      std::max({second.a.y, second.b.y, second.c.y}),
      std::max({second.a.z, second.b.z, second.c.z})};
  if (!overlaps(first_min, first_max, second_min, second_max)) return false;

  const Vec3 first_edges[][2] = {{first.a, first.b}, {first.b, first.c},
                                 {first.c, first.a}};
  const Vec3 second_edges[][2] = {{second.a, second.b}, {second.b, second.c},
                                  {second.c, second.a}};
  for (const auto& edge : first_edges) {
    if (segment_hits_triangle(edge[0], edge[1], second)) return true;
  }
  for (const auto& edge : second_edges) {
    if (segment_hits_triangle(edge[0], edge[1], first)) return true;
  }
  return false;
}

}  // namespace

double Mesh::compute_volume() const {
  double volume = 0.0;
  for (const Triangle& triangle : triangles) {
    volume += dot(triangle.a, cross(triangle.b, triangle.c)) / 6.0;
  }
  return volume;
}

bool ValidationResult::has_errors() const {
  for (const MeshDiagnostic& diagnostic : diagnostics) {
    if (diagnostic.severity == DiagnosticSeverity::ERROR) return true;
  }
  return false;
}

ValidationResult validate_and_normalize(const std::vector<Triangle>& triangles) {
  ValidationResult result;
  result.mesh.triangles = triangles;

  if (triangles.empty()) {
    add_diagnostic(result, DiagnosticSeverity::ERROR,
                   DiagnosticCategory::EMPTY_MESH, 0, 0,
                   "Mesh contains no triangles");
    return result;
  }

  Vec3 min = {std::numeric_limits<float>::max(),
              std::numeric_limits<float>::max(),
              std::numeric_limits<float>::max()};
  Vec3 max = {std::numeric_limits<float>::lowest(),
              std::numeric_limits<float>::lowest(),
              std::numeric_limits<float>::lowest()};
  std::unordered_map<EdgeKey, std::size_t, EdgeKeyHash> edges;
  std::unordered_map<VertexKey, std::vector<std::size_t>, VertexKeyHash>
      adjacency;

  for (std::size_t i = 0; i < triangles.size(); ++i) {
    const Triangle& triangle = triangles[i];
    const Vec3 points[] = {triangle.a, triangle.b, triangle.c};
    bool triangle_finite = finite(triangle.normal);
    for (const Vec3& point : points) {
      triangle_finite = triangle_finite && finite(point);
      min.x = std::min(min.x, point.x);
      min.y = std::min(min.y, point.y);
      min.z = std::min(min.z, point.z);
      max.x = std::max(max.x, point.x);
      max.y = std::max(max.y, point.y);
      max.z = std::max(max.z, point.z);
    }
    if (!triangle_finite) {
      add_diagnostic(result, DiagnosticSeverity::ERROR,
                     DiagnosticCategory::NONFINITE_COORDINATE, i, 1,
                     "Triangle contains a non-finite coordinate");
      continue;
    }

    const Vec3 cross_product = cross(subtract(triangle.b, triangle.a),
                                     subtract(triangle.c, triangle.a));
    const double area_twice = std::sqrt(dot(cross_product, cross_product));
    if (area_twice <= kAreaEpsilon) {
      add_diagnostic(result, DiagnosticSeverity::WARNING,
                     DiagnosticCategory::DEGENERATE_TRIANGLE, i, 1,
                     "Triangle has zero or near-zero area");
    }

    const VertexKey keys[] = {vertex_key(triangle.a), vertex_key(triangle.b),
                              vertex_key(triangle.c)};
    if (keys[0] == keys[1] || keys[1] == keys[2] || keys[0] == keys[2]) {
      add_diagnostic(result, DiagnosticSeverity::WARNING,
                     DiagnosticCategory::DUPLICATE_VERTEX, i, 1,
                     "Triangle contains duplicate vertices");
    }
    for (const VertexKey& key : keys) adjacency[key].push_back(i);
    ++edges[edge_key(keys[0], keys[1])];
    ++edges[edge_key(keys[1], keys[2])];
    ++edges[edge_key(keys[2], keys[0])];
  }

  if (result.has_errors()) {
    result.mesh.triangles.clear();
    return result;
  }

  result.mesh.min = min;
  result.mesh.max = max;
  const float z_offset = min.z;
  for (Triangle& triangle : result.mesh.triangles) {
    triangle.a.z -= z_offset;
    triangle.b.z -= z_offset;
    triangle.c.z -= z_offset;
  }
  result.mesh.min.z = 0.0f;
  result.mesh.max.z -= z_offset;

  std::size_t open_edges = 0;
  std::size_t non_manifold_edges = 0;
  for (const auto& entry : edges) {
    if (entry.second == 1) ++open_edges;
    if (entry.second > 2) ++non_manifold_edges;
  }
  if (open_edges != 0) {
    add_diagnostic(result, DiagnosticSeverity::WARNING,
                   DiagnosticCategory::OPEN_BOUNDARY, 0, open_edges,
                   "Mesh contains boundary edges");
  }
  if (non_manifold_edges != 0) {
    add_diagnostic(result, DiagnosticSeverity::WARNING,
                   DiagnosticCategory::NON_MANIFOLD_EDGE, 0,
                   non_manifold_edges, "Mesh contains non-manifold edges");
  }

  std::size_t self_intersections = 0;
  for (std::size_t i = 0; i < triangles.size(); ++i) {
    for (std::size_t j = i + 1; j < triangles.size(); ++j) {
      const VertexKey first_keys[] = {vertex_key(triangles[i].a),
                                      vertex_key(triangles[i].b),
                                      vertex_key(triangles[i].c)};
      const VertexKey second_keys[] = {vertex_key(triangles[j].a),
                                       vertex_key(triangles[j].b),
                                       vertex_key(triangles[j].c)};
      bool shares_vertex = false;
      for (const VertexKey& first_key : first_keys) {
        for (const VertexKey& second_key : second_keys) {
          shares_vertex = shares_vertex || first_key == second_key;
        }
      }
      if (!shares_vertex && triangles_intersect(triangles[i], triangles[j])) {
        ++self_intersections;
      }
    }
  }
  if (self_intersections != 0) {
    add_diagnostic(result, DiagnosticSeverity::WARNING,
                   DiagnosticCategory::SELF_INTERSECTION, 0,
                   self_intersections, "Mesh contains intersecting triangles");
  }

  std::unordered_set<std::size_t> visited;
  std::size_t components = 0;
  for (std::size_t i = 0; i < triangles.size(); ++i) {
    if (visited.count(i) != 0) continue;
    ++components;
    std::vector<std::size_t> pending = {i};
    visited.insert(i);
    while (!pending.empty()) {
      const std::size_t current = pending.back();
      pending.pop_back();
      for (const Vec3& point : {triangles[current].a, triangles[current].b,
                                triangles[current].c}) {
        for (std::size_t neighbour : adjacency[vertex_key(point)]) {
          if (visited.insert(neighbour).second) pending.push_back(neighbour);
        }
      }
    }
  }
  if (components > 1) {
    add_diagnostic(result, DiagnosticSeverity::WARNING,
                   DiagnosticCategory::DISCONNECTED_COMPONENT, 0, components,
                   "Mesh contains disconnected triangle components");
  }

  if (result.mesh.compute_volume() < -kAreaEpsilon) {
    add_diagnostic(result, DiagnosticSeverity::WARNING,
                   DiagnosticCategory::INVERTED_WINDING, 0, triangles.size(),
                   "Mesh winding produces a negative signed volume");
  }
  return result;
}

}  // namespace layer_cut
