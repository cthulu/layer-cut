#include "contour_builder.h"

#include <cmath>
#include <vector>

namespace {

using layer_cut::Contour;
using layer_cut::ContourDiagnosticCategory;
using layer_cut::PlaneSegment;
using layer_cut::Vec2;

PlaneSegment segment(double ax, double ay, double bx, double by) {
  return {{ax, ay}, {bx, by}, 0};
}

std::vector<PlaneSegment> rectangle(double min, double max) {
  return {segment(min, min, max, min), segment(max, min, max, max),
          segment(max, max, min, max), segment(min, max, min, min)};
}

bool has_category(const std::vector<layer_cut::ContourDiagnostic>& diagnostics,
                  ContourDiagnosticCategory category) {
  for (const auto& diagnostic : diagnostics) {
    if (diagnostic.category == category) return true;
  }
  return false;
}

}  // namespace

int main() {
  int failures = 0;

  // Shared endpoints that differ slightly are snapped into one closed loop.
  {
    auto input = rectangle(0.0, 10.0);
    input.push_back(segment(10.0 + 5e-7, 0.0, 10.0, 10.0));
    const auto result = layer_cut::build_contours(input);
    if (result.contours.size() != 1 || result.has_diagnostics() ||
        result.contours[0].hole ||
        std::abs(result.contours[0].signed_area - 100.0) > 1e-9) {
      ++failures;
    }
  }

  // Reversed duplicate edges are removed before graph traversal.
  {
    auto input = rectangle(0.0, 2.0);
    input.push_back(segment(2.0, 0.0, 0.0, 0.0));
    const auto result = layer_cut::build_contours(input);
    if (result.contours.size() != 1 || result.has_diagnostics()) ++failures;
  }

  // Nesting classifies the inner loop as a hole and normalizes winding.
  {
    auto input = rectangle(0.0, 10.0);
    auto inner = rectangle(2.0, 4.0);
    input.insert(input.end(), inner.begin(), inner.end());
    const auto result = layer_cut::build_contours(input);
    if (result.contours.size() != 2 || result.has_diagnostics()) {
      ++failures;
    } else {
      std::size_t holes = 0;
      for (const Contour& contour : result.contours) {
        if (contour.hole) {
          ++holes;
          if (std::abs(contour.signed_area + 4.0) > 1e-9) ++failures;
        } else if (std::abs(contour.signed_area - 100.0) > 1e-9) {
          ++failures;
        }
      }
      if (holes != 1) ++failures;
    }
  }

  // Disconnected loops remain separate contours.
  {
    auto input = rectangle(0.0, 1.0);
    auto second = rectangle(5.0, 7.0);
    input.insert(input.end(), second.begin(), second.end());
    const auto result = layer_cut::build_contours(input);
    if (result.contours.size() != 2 || result.has_diagnostics()) ++failures;
  }

  // Dangling and branching graphs are diagnosed rather than silently joined.
  {
    const auto dangling = layer_cut::build_contours(
        {segment(0, 0, 1, 0), segment(1, 0, 1, 1)});
    if (!has_category(dangling.diagnostics,
                     ContourDiagnosticCategory::DANGLING_EDGE) ||
        !has_category(dangling.diagnostics,
                      ContourDiagnosticCategory::OPEN_CONTOUR)) {
      ++failures;
    }
  }
  {
    const auto branching = layer_cut::build_contours(
        {segment(0, 0, 1, 0), segment(0, 0, -1, 0), segment(0, 0, 0, 1)});
    if (!has_category(branching.diagnostics,
                     ContourDiagnosticCategory::BRANCHING_VERTEX)) {
      ++failures;
    }
  }

  return failures;
}
