#pragma once

#include "mesh.h"

#include <cstdint>
#include <vector>

namespace layer_cut {

struct Vec2 {
  double x, y;
};

struct PlaneSegment {
  Vec2 a, b;
  // Bits identify triangle edges AB, BC, and CA which contributed to this
  // segment. Endpoints remain the canonical identity for later deduplication.
  uint8_t source_edge_mask = 0;
};

struct PlaneIntersectionOptions {
  double epsilon = 1e-6;
};

// Intersect one triangle with the horizontal plane z=plane_z. Coplanar
// triangles return their three boundary edges; a point touch returns none.
std::vector<PlaneSegment> intersect_triangle_with_plane(
    const Triangle& triangle, double plane_z,
    const PlaneIntersectionOptions& options = PlaneIntersectionOptions());

}  // namespace layer_cut
