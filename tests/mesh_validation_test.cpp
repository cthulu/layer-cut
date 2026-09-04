#include "mesh.h"
#include "stl_loader.h"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

namespace {

using layer_cut::DiagnosticCategory;
using layer_cut::MeshDiagnostic;
using layer_cut::Triangle;
using layer_cut::Vec3;

Triangle triangle(Vec3 normal, Vec3 a, Vec3 b, Vec3 c) {
  return {normal, a, b, c};
}

std::vector<Triangle> cube() {
  const float low = -10.0f;
  const float high = 10.0f;
  return {
      triangle({0, 0, -1}, {low, low, low}, {low, high, low}, {high, high, low}),
      triangle({0, 0, -1}, {low, low, low}, {high, high, low}, {high, low, low}),
      triangle({0, 0, 1}, {low, low, high}, {high, high, high}, {low, high, high}),
      triangle({0, 0, 1}, {low, low, high}, {high, low, high}, {high, high, high}),
      triangle({-1, 0, 0}, {low, low, low}, {low, high, high}, {low, high, low}),
      triangle({-1, 0, 0}, {low, low, low}, {low, low, high}, {low, high, high}),
      triangle({1, 0, 0}, {high, low, low}, {high, high, low}, {high, high, high}),
      triangle({1, 0, 0}, {high, low, low}, {high, high, high}, {high, low, high}),
      triangle({0, -1, 0}, {low, low, low}, {high, low, low}, {high, low, high}),
      triangle({0, -1, 0}, {low, low, low}, {high, low, high}, {low, low, high}),
      triangle({0, 1, 0}, {low, high, low}, {low, high, high}, {high, high, high}),
      triangle({0, 1, 0}, {low, high, low}, {high, high, high}, {high, high, low}),
  };
}

bool has_category(const std::vector<MeshDiagnostic>& diagnostics,
                  DiagnosticCategory category) {
  for (const MeshDiagnostic& diagnostic : diagnostics) {
    if (diagnostic.category == category) return true;
  }
  return false;
}

}  // namespace

int main() {
  int failures = 0;

  // A closed cube is the exact geometry baseline for normalization.
  {
    const auto result = layer_cut::validate_and_normalize(cube());
    if (result.has_errors() || result.mesh.triangles.size() != 12 ||
        has_category(result.diagnostics, DiagnosticCategory::OPEN_BOUNDARY) ||
        has_category(result.diagnostics, DiagnosticCategory::NON_MANIFOLD_EDGE)) {
      ++failures;
    }
    if (std::abs(result.mesh.min.x + 10.0f) > 1e-6f ||
        std::abs(result.mesh.min.y + 10.0f) > 1e-6f ||
        std::abs(result.mesh.min.z) > 1e-6f ||
        std::abs(result.mesh.max.x - 10.0f) > 1e-6f ||
        std::abs(result.mesh.max.y - 10.0f) > 1e-6f ||
        std::abs(result.mesh.max.z - 20.0f) > 1e-6f ||
        std::abs(result.mesh.compute_volume() - 8000.0) > 1e-6) {
      ++failures;
    }
  }

  // Normalization must preserve X/Y and translate only Z.
  {
    const std::vector<Triangle> input = {
        triangle({0, 0, 1}, {-2, 3, -5}, {4, 3, -5}, {-2, 8, 5})};
    const auto result = layer_cut::validate_and_normalize(input);
    const Triangle& normalized = result.mesh.triangles[0];
    if (normalized.a.x != input[0].a.x || normalized.a.y != input[0].a.y ||
        std::abs(normalized.a.z) > 1e-6f ||
        std::abs(normalized.c.z - 10.0f) > 1e-6f) {
      ++failures;
    }
  }

  // Unsafe topology is retained for best-effort processing and diagnosed.
  {
    const std::vector<Triangle> input = {
        triangle({0, 0, 1}, {0, 0, 0}, {0, 0, 0}, {1, 0, 0})};
    const auto result = layer_cut::validate_and_normalize(input);
    if (result.has_errors() ||
        !has_category(result.diagnostics, DiagnosticCategory::DEGENERATE_TRIANGLE) ||
        !has_category(result.diagnostics, DiagnosticCategory::DUPLICATE_VERTEX) ||
        !has_category(result.diagnostics, DiagnosticCategory::OPEN_BOUNDARY)) {
      ++failures;
    }
  }

  // Non-finite coordinates are rejected before normalization.
  {
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const auto result = layer_cut::validate_and_normalize(
        {triangle({0, 0, 1}, {nan, 0, 0}, {1, 0, 0}, {0, 1, 0})});
    if (!result.has_errors() ||
        !has_category(result.diagnostics, DiagnosticCategory::NONFINITE_COORDINATE)) {
      ++failures;
    }
  }

  // Disconnected and non-manifold topology is diagnosed without rejection.
  {
    const std::vector<Triangle> disconnected = {
        triangle({0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {0, 1, 0}),
        triangle({0, 0, 1}, {10, 0, 0}, {11, 0, 0}, {10, 1, 0})};
    const auto result = layer_cut::validate_and_normalize(disconnected);
    if (result.has_errors() ||
        !has_category(result.diagnostics,
                      DiagnosticCategory::DISCONNECTED_COMPONENT)) {
      ++failures;
    }
  }

  {
    const std::vector<Triangle> non_manifold = {
        triangle({0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {0, 1, 0}),
        triangle({0, 0, 1}, {1, 0, 0}, {0, 0, 0}, {0, -1, 0}),
        triangle({0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {0, 0, 1})};
    const auto result = layer_cut::validate_and_normalize(non_manifold);
    if (result.has_errors() ||
        !has_category(result.diagnostics,
                      DiagnosticCategory::NON_MANIFOLD_EDGE)) {
      ++failures;
    }
  }

  {
    const std::vector<Triangle> intersecting = {
        triangle({0, 0, 1}, {-1, 0, 0}, {1, 0, 0}, {0, 1, 0}),
        triangle({0, 0, 1}, {0, -1, -1}, {0, -1, 1}, {0, 1, 0.5f})};
    const auto result = layer_cut::validate_and_normalize(intersecting);
    if (result.has_errors() ||
        !has_category(result.diagnostics,
                      DiagnosticCategory::SELF_INTERSECTION)) {
      ++failures;
    }
  }

  // The supplied real-world model is a loader-to-validation regression test.
  {
    layer_cut::LoadError error;
    const auto triangles = layer_cut::load_stl(
        std::string(LAYER_CUT_SOURCE_DIR) + "/testfiles/fox.stl",
        layer_cut::LoadLimits(), &error);
    const auto result = layer_cut::validate_and_normalize(triangles);
    if (error.code != layer_cut::LoadError::Code::OK || triangles.size() != 376 ||
        result.has_errors() || std::abs(result.mesh.min.z) > 1e-6f ||
        result.mesh.max.z < 106.0f) {
      ++failures;
    }
  }

  return failures;
}
