#pragma once

#include "mesh.h"
#include "slicer.h"

#include <string>
#include <vector>

namespace layer_cut {

struct StackedPreviewResult {
  std::vector<Triangle> triangles;
  std::string error;

  bool ok() const { return error.empty(); }
};

// Build one watertight thin solid for every prepared slice layer. The layer
// contours must already have gone through the same union/cleanup pipeline used
// for cutting.
StackedPreviewResult make_stacked_preview(
    const std::vector<SliceLayer>& layers, double layer_height);

}  // namespace layer_cut
