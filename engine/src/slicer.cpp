#include "slicer.h"

#include "plane_intersection.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <utility>
#include <vector>

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

  // Index triangles by Z extent once so each layer only intersects the
  // triangles that actually span its plane instead of scanning the full mesh.
  const std::size_t triangle_count = mesh.triangles.size();
  std::vector<float> triangle_min_z(triangle_count);
  std::vector<float> triangle_max_z(triangle_count);
  for (std::size_t i = 0; i < triangle_count; ++i) {
    const Triangle& triangle = mesh.triangles[i];
    triangle_min_z[i] = std::min({triangle.a.z, triangle.b.z, triangle.c.z});
    triangle_max_z[i] = std::max({triangle.a.z, triangle.b.z, triangle.c.z});
  }
  std::vector<std::size_t> by_min_z(triangle_count);
  std::iota(by_min_z.begin(), by_min_z.end(), std::size_t{0});
  std::stable_sort(by_min_z.begin(), by_min_z.end(),
                   [&](std::size_t first, std::size_t second) {
                     return triangle_min_z[first] < triangle_min_z[second];
                   });

  std::size_t active_start = 0;
  for (std::size_t index = 0; index < layer_count; ++index) {
    const double z = mesh.min.z + (static_cast<double>(index) + 0.5) *
                                      config.layer_height;
    if (z >= mesh.max.z - config.epsilon) continue;
    // Retire triangles ending below this plane; layer Z increases
    // monotonically, so retired triangles cannot produce segments again.
    while (active_start < triangle_count &&
           static_cast<double>(triangle_max_z[by_min_z[active_start]]) <
               z - config.epsilon) {
      ++active_start;
    }
    std::vector<PlaneSegment> segments;
    for (std::size_t candidate = active_start; candidate < triangle_count;
         ++candidate) {
      const std::size_t t = by_min_z[candidate];
      if (static_cast<double>(triangle_min_z[t]) > z + config.epsilon) break;
      if (static_cast<double>(triangle_max_z[t]) < z - config.epsilon) {
        continue;
      }
      const auto triangle_segments = intersect_triangle_with_plane(
          mesh.triangles[t], z, {config.epsilon});
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
