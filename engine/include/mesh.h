#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace layer_cut {

struct Vec3 {
  float x, y, z;
};

struct Triangle {
  Vec3 normal;
  Vec3 a, b, c;
};

struct Mesh {
  std::vector<Triangle> triangles;
  Vec3 min{0.0f, 0.0f, 0.0f};
  Vec3 max{0.0f, 0.0f, 0.0f};

  // Signed volume in cubic millimetres. Open or invalid meshes are not
  // guaranteed to have a meaningful volume.
  double compute_volume() const;
};

enum class DiagnosticSeverity {
  WARNING,
  ERROR,
};

enum class DiagnosticCategory {
  EMPTY_MESH,
  NONFINITE_COORDINATE,
  DEGENERATE_TRIANGLE,
  DUPLICATE_VERTEX,
  OPEN_BOUNDARY,
  NON_MANIFOLD_EDGE,
  SELF_INTERSECTION,
  INVERTED_WINDING,
  DISCONNECTED_COMPONENT,
};

struct MeshDiagnostic {
  DiagnosticSeverity severity;
  DiagnosticCategory category;
  std::size_t triangle_index = 0;
  std::size_t affected_count = 0;
  std::string message;
};

struct ValidationResult {
  Mesh mesh;
  std::vector<MeshDiagnostic> diagnostics;

  bool has_errors() const;
};

// Validate geometry and return a copy translated so min-Z is zero. Invalid
// topology is reported but retained for best-effort downstream operations.
ValidationResult validate_and_normalize(const std::vector<Triangle>& triangles);

}  // namespace layer_cut
