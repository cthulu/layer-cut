#include "slicer.h"
#include "stl_loader.h"

#include <cmath>
#include <vector>
#include <string>

namespace {

using layer_cut::Triangle;
using layer_cut::Vec3;

Triangle t(Vec3 normal, Vec3 a, Vec3 b, Vec3 c) { return {normal, a, b, c}; }

std::vector<Triangle> cube() {
  const float lo = 0.0f;
  const float hi = 20.0f;
  return {
      t({0, 0, -1}, {lo, lo, lo}, {hi, hi, lo}, {lo, hi, lo}),
      t({0, 0, -1}, {lo, lo, lo}, {hi, lo, lo}, {hi, hi, lo}),
      t({0, 0, 1}, {lo, lo, hi}, {lo, hi, hi}, {hi, hi, hi}),
      t({0, 0, 1}, {lo, lo, hi}, {hi, hi, hi}, {hi, lo, hi}),
      t({-1, 0, 0}, {lo, lo, lo}, {lo, hi, lo}, {lo, hi, hi}),
      t({-1, 0, 0}, {lo, lo, lo}, {lo, hi, hi}, {lo, lo, hi}),
      t({1, 0, 0}, {hi, lo, lo}, {hi, lo, hi}, {hi, hi, hi}),
      t({1, 0, 0}, {hi, lo, lo}, {hi, hi, hi}, {hi, hi, lo}),
      t({0, -1, 0}, {lo, lo, lo}, {lo, lo, hi}, {hi, lo, hi}),
      t({0, -1, 0}, {lo, lo, lo}, {hi, lo, hi}, {hi, lo, lo}),
      t({0, 1, 0}, {lo, hi, lo}, {hi, hi, hi}, {lo, hi, hi}),
      t({0, 1, 0}, {lo, hi, lo}, {hi, hi, lo}, {hi, hi, hi}),
  };
}

}  // namespace

int main() {
  int failures = 0;
  layer_cut::Mesh mesh;
  mesh.triangles = cube();
  mesh.min = {0, 0, 0};
  mesh.max = {20, 20, 20};
  const auto result = layer_cut::slice_mesh(mesh);
  if (!result.valid() || result.layers.size() != 100) ++failures;
  for (const auto& layer : result.layers) {
    if (layer.contours.size() != 1 || layer.contours[0].hole ||
        std::abs(layer.contours[0].signed_area - 400.0) > 1e-6 ||
        std::abs(layer.z - (static_cast<double>(layer.index) + 0.5) * 0.2) >
            1e-9) {
      ++failures;
    }
  }
  if (layer_cut::slice_mesh(mesh, {0.0, 1e-6}).valid()) ++failures;

  // Exercise the complete real-model path used by the CLI.
  {
    layer_cut::LoadError error;
    const auto triangles = layer_cut::load_stl(
        std::string(LAYER_CUT_SOURCE_DIR) + "/testfiles/fox.stl",
        layer_cut::LoadLimits(), &error);
    const auto validation = layer_cut::validate_and_normalize(triangles);
    const auto fox = layer_cut::slice_mesh(validation.mesh);
    std::size_t non_empty_layers = 0;
    for (const auto& layer : fox.layers) {
      if (!layer.contours.empty()) ++non_empty_layers;
    }
    if (error.code != layer_cut::LoadError::Code::OK || validation.has_errors() ||
        !fox.valid() || fox.layers.size() != 533 || non_empty_layers == 0) {
      ++failures;
    }
  }
  return failures;
}
