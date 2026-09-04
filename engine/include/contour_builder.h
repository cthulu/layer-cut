#pragma once

#include "plane_intersection.h"

#include <cstddef>
#include <vector>

namespace layer_cut {

struct Contour {
  std::vector<Vec2> points;
  bool hole = false;
  double signed_area = 0.0;
};

enum class ContourDiagnosticCategory {
  DANGLING_EDGE,
  BRANCHING_VERTEX,
  OPEN_CONTOUR,
};

struct ContourDiagnostic {
  ContourDiagnosticCategory category;
  std::size_t affected_count = 0;
};

struct ContourBuildOptions {
  double snap_tolerance = 1e-6;
};

struct ContourBuildResult {
  std::vector<Contour> contours;
  std::vector<ContourDiagnostic> diagnostics;

  bool has_diagnostics() const { return !diagnostics.empty(); }
};

// Snap, deduplicate, and join plane segments into closed contours. Outer
// contours are counter-clockwise; holes are clockwise.
ContourBuildResult build_contours(
    const std::vector<PlaneSegment>& segments,
    const ContourBuildOptions& options = ContourBuildOptions());

}  // namespace layer_cut
