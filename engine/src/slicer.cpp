#include "slicer.h"

#include "plane_intersection.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace layer_cut {

SliceResult slice_mesh(const Mesh& mesh, const SliceConfig& config) {
  SliceResult result;
  if (!std::isfinite(config.layer_height) || config.layer_height <= 0.0) {
    result.errors.push_back("Layer height must be positive and finite");
    return result;
  }
  if (!std::isfinite(config.epsilon) || config.epsilon < 0.0) {
    result.errors.push_back("Slice epsilon must be finite and non-negative");
    return result;
  }
  if (mesh.triangles.empty() || !std::isfinite(mesh.min.z) ||
      !std::isfinite(mesh.max.z) || mesh.max.z <= mesh.min.z) {
    result.errors.push_back("Mesh has no positive Z extent");
    return result;
  }

  const double height = static_cast<double>(mesh.max.z) - mesh.min.z;
  const double ratio = height / config.layer_height;
  const std::size_t layer_count = static_cast<std::size_t>(std::ceil(
      std::max(0.0, ratio - 0.5 - config.epsilon)));
  for (std::size_t index = 0; index < layer_count; ++index) {
    const double z = mesh.min.z + (static_cast<double>(index) + 0.5) *
                                      config.layer_height;
    if (z >= mesh.max.z - config.epsilon) continue;
    std::vector<PlaneSegment> segments;
    for (const Triangle& triangle : mesh.triangles) {
      const auto triangle_segments = intersect_triangle_with_plane(
          triangle, z, {config.epsilon});
      segments.insert(segments.end(), triangle_segments.begin(),
                     triangle_segments.end());
    }
    const ContourBuildResult built = build_contours(segments);
    SliceLayer layer;
    layer.index = index;
    layer.z = z;
    layer.min = {mesh.min.x, mesh.min.y};
    layer.max = {mesh.max.x, mesh.max.y};
    layer.contours = built.contours;
    layer.diagnostics = built.diagnostics;
    result.layers.push_back(std::move(layer));
  }
  return result;
}

}  // namespace layer_cut
