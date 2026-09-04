#include "plane_intersection.h"

#include <cmath>
#include <vector>

namespace {

using layer_cut::PlaneSegment;
using layer_cut::Triangle;
using layer_cut::Vec3;

Triangle triangle(Vec3 a, Vec3 b, Vec3 c) {
  return {{0, 0, 0}, a, b, c};
}

bool near(double actual, double expected) {
  return std::abs(actual - expected) <= 1e-9;
}

bool endpoint_pair_is(const PlaneSegment& segment, double ax, double ay,
                      double bx, double by) {
  const bool forward = near(segment.a.x, ax) && near(segment.a.y, ay) &&
                       near(segment.b.x, bx) && near(segment.b.y, by);
  const bool reverse = near(segment.a.x, bx) && near(segment.a.y, by) &&
                       near(segment.b.x, ax) && near(segment.b.y, ay);
  return forward || reverse;
}

}  // namespace

int main() {
  int failures = 0;
  const auto intersect = [](const Triangle& triangle, double z) {
    return layer_cut::intersect_triangle_with_plane(triangle, z);
  };

  // One vertex below and two above produces two interpolated endpoints.
  {
    const auto segments = intersect(
        triangle({0, 0, -1}, {2, 0, 1}, {0, 2, 1}), 0.0);
    if (segments.size() != 1 || !endpoint_pair_is(segments[0], 1.0, 0.0, 0.0, 1.0) ||
        segments[0].source_edge_mask != 5) {
      ++failures;
    }
  }

  // Two vertices below and one above also produce one segment.
  {
    const auto segments = intersect(
        triangle({0, 0, -1}, {2, 0, -1}, {0, 2, 1}), 0.0);
    if (segments.size() != 1 || !endpoint_pair_is(segments[0], 0.0, 1.0, 1.0, 1.0) ||
        segments[0].source_edge_mask != 6) {
      ++failures;
    }
  }

  // A single vertex touch has zero-length intersection and emits no segment.
  {
    if (!intersect(triangle({0, 0, 0}, {1, 0, 1}, {0, 1, 1}), 0.0).empty()) {
      ++failures;
    }
  }

  // An edge on the plane is returned once, even though both endpoints are on it.
  {
    const auto segments = intersect(
        triangle({0, 0, 0}, {2, 0, 0}, {0, 2, 1}), 0.0);
    if (segments.size() != 1 || !endpoint_pair_is(segments[0], 0.0, 0.0, 2.0, 0.0) ||
        segments[0].source_edge_mask != 1) {
      ++failures;
    }
  }

  // A fully coplanar triangle returns its three distinct boundary edges.
  {
    const auto segments = intersect(
        triangle({0, 0, 0}, {2, 0, 0}, {0, 2, 0}), 0.0);
    if (segments.size() != 3) ++failures;
  }

  // A triangle wholly on one side has no intersection.
  {
    if (!intersect(triangle({0, 0, 1}, {1, 0, 1}, {0, 1, 1}), 0.0).empty()) {
      ++failures;
    }
  }

  // Near-plane vertices use the documented epsilon classification.
  {
    const auto segments = layer_cut::intersect_triangle_with_plane(
        triangle({0, 0, 1e-7f}, {2, 0, 1}, {0, 2, -1}), 0.0);
    if (segments.size() != 1 || !endpoint_pair_is(segments[0], 0.0, 0.0, 1.0, 1.0)) {
      ++failures;
    }
  }

  // Invalid plane parameters fail closed.
  {
    if (!layer_cut::intersect_triangle_with_plane(
                  triangle({0, 0, 0}, {1, 0, 1}, {0, 1, -1}), NAN)
                 .empty()) {
      ++failures;
    }
  }

  return failures;
}
