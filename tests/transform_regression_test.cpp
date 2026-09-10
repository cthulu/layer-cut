#include "layer_cut.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Point { float x, y, z; };

void write_u16(std::ofstream& out, uint16_t value) { out.write(reinterpret_cast<const char*>(&value), 2); }
void write_u32(std::ofstream& out, uint32_t value) { out.write(reinterpret_cast<const char*>(&value), 4); }
void write_f32(std::ofstream& out, float value) { out.write(reinterpret_cast<const char*>(&value), 4); }

void write_triangle(std::ofstream& out, Point a, Point b, Point c) {
  write_f32(out, 0); write_f32(out, 0); write_f32(out, 0);
  for (Point p : {a, b, c}) { write_f32(out, p.x); write_f32(out, p.y); write_f32(out, p.z); }
  write_u16(out, 0);
}

Point orient(Point p, int axis);

Point transform(Point p, int axis, double degrees) {
  p = orient(p, axis);
  const double radians = degrees * 3.14159265358979323846 / 180.0;
  const float c = static_cast<float>(std::cos(radians));
  const float s = static_cast<float>(std::sin(radians));
  return {p.x * c - p.y * s, p.x * s + p.y * c, p.z};
}

void write_cuboid(const std::string& path, int axis = 4, double degrees = 0) {
  const float x = 20, y = 30, z = 40;
  const std::array<Point, 8> p = {{{0, 0, 0}, {x, 0, 0}, {x, y, 0}, {0, y, 0},
                                   {0, 0, z}, {x, 0, z}, {x, y, z}, {0, y, z}}};
  const std::array<std::array<int, 3>, 12> faces = {{{0, 2, 3}, {0, 1, 2},
      {4, 7, 6}, {4, 6, 5}, {0, 3, 7}, {0, 7, 4}, {1, 5, 6}, {1, 6, 2},
      {0, 4, 5}, {0, 5, 1}, {3, 6, 7}, {3, 2, 6}}};
  std::ofstream out(path, std::ios::binary);
  char header[80] = {};
  std::memcpy(header, "layer-cut transform probe", 26);
  out.write(header, sizeof(header));
  write_u32(out, static_cast<uint32_t>(faces.size()));
  float minimum_z = INFINITY;
  for (Point point : p) minimum_z = std::min(minimum_z, transform(point, axis, degrees).z);
  for (auto face : faces) {
    Point a = transform(p[face[0]], axis, degrees);
    Point b = transform(p[face[1]], axis, degrees);
    Point c = transform(p[face[2]], axis, degrees);
    a.z -= minimum_z; b.z -= minimum_z; c.z -= minimum_z;
    write_triangle(out, a, b, c);
  }
}

Point orient(Point p, int axis) {
  switch (axis) {
    case 0: return {p.z, p.y, -p.x};
    case 1: return {-p.z, p.y, p.x};
    case 2: return {p.x, p.z, -p.y};
    case 3: return {p.x, -p.z, p.y};
    case 5: return {p.x, -p.y, -p.z};
    default: return p;
  }
}

slicer_bounds_t expected_bounds(int axis) {
  constexpr double radians = 45.0 * 3.14159265358979323846 / 180.0;
  const double c = std::cos(radians), s = std::sin(radians);
  slicer_bounds_t bounds{INFINITY, INFINITY, INFINITY, -INFINITY, -INFINITY, -INFINITY};
  const std::array<Point, 8> corners = {{{0, 0, 0}, {20, 0, 0}, {0, 30, 0}, {20, 30, 0},
                                          {0, 0, 40}, {20, 0, 40}, {0, 30, 40}, {20, 30, 40}}};
  for (Point source : corners) {
    const Point p = orient(source, axis);
    const Point q{static_cast<float>(p.x * c - p.y * s),
                  static_cast<float>(p.x * s + p.y * c), p.z};
    bounds.min_x = std::min(bounds.min_x, q.x); bounds.max_x = std::max(bounds.max_x, q.x);
    bounds.min_y = std::min(bounds.min_y, q.y); bounds.max_y = std::max(bounds.max_y, q.y);
    bounds.min_z = std::min(bounds.min_z, q.z); bounds.max_z = std::max(bounds.max_z, q.z);
  }
  const float z = bounds.min_z;
  bounds.min_z = 0; bounds.max_z -= z;
  return bounds;
}

bool close(float actual, float expected) { return std::abs(actual - expected) < 0.001f; }

uint64_t hash_bytes(const uint8_t* bytes, size_t size) {
  uint64_t hash = 1469598103934665603ULL;
  for (size_t i = 0; i < size; ++i) { hash ^= bytes[i]; hash *= 1099511628211ULL; }
  return hash;
}

void write_bytes(const std::string& path, const uint8_t* bytes, size_t size) {
  std::ofstream out(path, std::ios::binary);
  out.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(size));
}

bool check_bounds(const slicer_bounds_t& actual, const slicer_bounds_t& expected) {
  return close(actual.min_x, expected.min_x) && close(actual.min_y, expected.min_y) &&
         close(actual.min_z, expected.min_z) && close(actual.max_x, expected.max_x) &&
         close(actual.max_y, expected.max_y) && close(actual.max_z, expected.max_z);
}

}  // namespace

int main() {
  const std::string path = std::string(LAYER_CUT_SOURCE_DIR) + "/testfiles/fox.stl";
  slicer_mesh_t mesh = slicer_load_stl(path.c_str());
  if (!mesh) { std::cerr << slicer_last_error() << '\n'; return 1; }

  slicer_config_t baseline_config = slicer_config_create();
  slicer_config_set_layer_height(baseline_config, 1.0);
  slicer_config_set_cleanup_mode(baseline_config, 1);
  slicer_config_set_transform(baseline_config, 4, 0, 1);
  slicer_result_t baseline = slicer_slice(mesh, baseline_config);
  if (!baseline) { std::cerr << slicer_last_error() << '\n'; return 2; }
  const char* baseline_svg = slicer_result_layer_svg(baseline, 0);
  const size_t baseline_svg_size = slicer_result_layer_svg_size(baseline, 0);
  size_t baseline_stl_size = 0;
  const uint8_t* baseline_stl = slicer_result_stacked_stl(baseline, &baseline_stl_size);
  if (!baseline_svg || baseline_svg_size == 0) { std::cerr << "baseline SVG output missing\n"; return 3; }
  if (!baseline_stl || baseline_stl_size == 0)
    std::cerr << "warning: baseline stacked STL output is unavailable\n";
  const uint64_t baseline_svg_hash = hash_bytes(reinterpret_cast<const uint8_t*>(baseline_svg), baseline_svg_size);
  const uint64_t baseline_stl_hash = baseline_stl ? hash_bytes(baseline_stl, baseline_stl_size) : 0;
  if (baseline_stl && baseline_stl_size > 0)
    write_bytes("/tmp/layer-cut-transform-fox-baseline.stl", baseline_stl, baseline_stl_size);
  std::cout << "baseline layers=" << slicer_result_layer_count(baseline)
            << " svg_hash=" << baseline_svg_hash << " stl_hash=" << baseline_stl_hash << '\n';

  int failures = 0;
  for (int axis = 0; axis < 6; ++axis) {
    slicer_config_t config = slicer_config_create();
    slicer_config_set_layer_height(config, 1.0);
    slicer_config_set_cleanup_mode(config, 1);
    slicer_config_set_transform(config, axis, 45.0, 1.0);
    slicer_result_t result = slicer_slice(mesh, config);
    slicer_snapshot_t snapshot = nullptr;
    slicer_bounds_t bounds{};
    const bool sliced = result && slicer_mesh_snapshot(mesh, axis, 45.0, 1.0, &snapshot) &&
                        slicer_snapshot_bounds(snapshot, &bounds);
    const char* svg = result ? slicer_result_layer_svg(result, 0) : nullptr;
    const size_t svg_size = result ? slicer_result_layer_svg_size(result, 0) : 0;
    size_t stl_size = 0;
    const uint8_t* stl = result ? slicer_result_stacked_stl(result, &stl_size) : nullptr;
    const bool output = svg && svg_size > 0;
    const bool stl_different = stl && stl_size > 0 && baseline_stl && baseline_stl_size > 0 &&
                               hash_bytes(stl, stl_size) != baseline_stl_hash;
    const bool different = output && (hash_bytes(reinterpret_cast<const uint8_t*>(svg), svg_size) != baseline_svg_hash || stl_different);
    const bool valid = sliced && std::isfinite(bounds.min_x) && std::isfinite(bounds.max_x) &&
                       std::isfinite(bounds.min_y) && std::isfinite(bounds.max_y) &&
                       std::isfinite(bounds.min_z) && std::isfinite(bounds.max_z) && different;
    std::cout << "axis=" << axis << " layers=" << (result ? slicer_result_layer_count(result) : 0)
              << " bounds=" << bounds.min_x << ',' << bounds.min_y << ',' << bounds.min_z << ".."
              << bounds.max_x << ',' << bounds.max_y << ',' << bounds.max_z
              << " stacked=" << stl_size << " different=" << (different ? "yes" : "no")
              << " result=" << (valid ? "PASS" : "FAIL") << '\n';
    if (result) {
      for (int i = 0; i < slicer_result_diagnostic_count(result); ++i)
        std::cout << "  diagnostic: " << slicer_result_diagnostic(result, i) << '\n';
    }
    if (stl && stl_size > 0)
      write_bytes("/tmp/layer-cut-transform-fox-axis-" + std::to_string(axis) + "-45.stl", stl, stl_size);
    if (!valid) ++failures;
    if (snapshot) slicer_free_snapshot(snapshot);
    if (result) slicer_free_result(result);
    slicer_free_config(config);
  }
  slicer_free_result(baseline);
  slicer_free_config(baseline_config);
  slicer_free_mesh(mesh);
  std::cout << "STL files retained under /tmp/layer-cut-transform-fox-*.stl\n";
  return failures == 0 ? 0 : 3;
}
