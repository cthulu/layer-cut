#include "layer_cut.h"

#include <array>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
struct P { float x, y, z; };
void u16(std::ofstream& out, uint16_t v) { out.write(reinterpret_cast<char*>(&v), 2); }
void u32(std::ofstream& out, uint32_t v) { out.write(reinterpret_cast<char*>(&v), 4); }
void f32(std::ofstream& out, float v) { out.write(reinterpret_cast<char*>(&v), 4); }
void tri(std::ofstream& out, P a, P b, P c) {
  f32(out, 0); f32(out, 0); f32(out, 0);
  for (P p : {a, b, c}) { f32(out, p.x); f32(out, p.y); f32(out, p.z); }
  u16(out, 0);
}
std::string make_cuboid() {
  const std::string path = "/tmp/layer-cut-asymmetric-transform.stl";
  const std::array<P, 8> p = {{{0,0,0},{20,0,0},{20,30,0},{0,30,0},
                                {0,0,40},{20,0,40},{20,30,40},{0,30,40}}};
  const std::array<std::array<int,3>,12> faces = {{{0,2,3},{0,1,2},{4,7,6},{4,6,5},
    {0,3,7},{0,7,4},{1,5,6},{1,6,2},{0,4,5},{0,5,1},{3,6,7},{3,2,6}}};
  std::ofstream out(path, std::ios::binary); char header[80] = {};
  std::memcpy(header, "layer-cut asymmetric transform", 31); out.write(header, 80);
  u32(out, static_cast<uint32_t>(faces.size()));
  for (auto face : faces) tri(out, p[face[0]], p[face[1]], p[face[2]]);
  return path;
}
bool close(float a, float b) { return std::abs(a - b) < 0.002f; }
bool same(const slicer_bounds_t& a, const slicer_bounds_t& b) {
  return close(a.min_x,b.min_x) && close(a.min_y,b.min_y) && close(a.min_z,b.min_z) &&
         close(a.max_x,b.max_x) && close(a.max_y,b.max_y) && close(a.max_z,b.max_z);
}
bool finite_bounds(const slicer_bounds_t& b) {
  return std::isfinite(b.min_x) && std::isfinite(b.min_y) && std::isfinite(b.min_z) &&
         std::isfinite(b.max_x) && std::isfinite(b.max_y) && std::isfinite(b.max_z);
}
bool has_vertex(const std::vector<float>& values, float x, float y, float z) {
  for (size_t i = 0; i + 2 < values.size(); i += 3) {
    if (close(values[i], x) && close(values[i + 1], y) && close(values[i + 2], z)) return true;
  }
  return false;
}
}

int main() {
  const std::string path = make_cuboid();
  slicer_mesh_t mesh = slicer_load_stl(path.c_str());
  if (!mesh) { std::cerr << slicer_last_error() << '\n'; return 1; }
  slicer_bounds_t source{}; slicer_mesh_bounds(mesh, &source);
  slicer_config_t config = slicer_config_create();
  slicer_config_set_layer_height(config, 5.0);
  slicer_config_set_cleanup_mode(config, 0);
  slicer_result_t identity = slicer_slice(mesh, config);
  if (!identity || slicer_result_layer_count(identity) != 8 ||
      !close(source.max_x, 20) || !close(source.max_y, 30) || !close(source.max_z, 40)) return 2;

  slicer_snapshot_t snapshot = nullptr; slicer_bounds_t identity_snapshot{};
  if (!slicer_mesh_snapshot_euler(mesh, 4, 0, 0, 0, 1, &snapshot) ||
      !slicer_snapshot_bounds(snapshot, &identity_snapshot) || !same(source, identity_snapshot)) return 3;
  slicer_free_snapshot(snapshot);

  const std::array<float, 6> expected_axis_heights = {20, 20, 30, 30, 40, 40};
  for (int axis = 0; axis < 6; ++axis) {
    if (!slicer_mesh_snapshot_euler(mesh, axis, 0, 0, 0, 1, &snapshot)) return 4;
    slicer_bounds_t axis_bounds{};
    if (!slicer_snapshot_bounds(snapshot, &axis_bounds) ||
        !close(axis_bounds.min_z, 0) ||
        !close(axis_bounds.max_z, expected_axis_heights[static_cast<size_t>(axis)])) return 4;
    slicer_free_snapshot(snapshot);
  }

  // Signed axis orientation maps the selected positive model direction to +Z.
  if (!slicer_mesh_snapshot_euler(mesh, 0, 0, 0, 0, 1, &snapshot)) return 14;
  size_t vertex_count = 0;
  const float* vertex_data = slicer_snapshot_vertices(snapshot, &vertex_count);
  std::vector<float> vertices(vertex_data, vertex_data + vertex_count);
  if (!has_vertex(vertices, -40, 0, 0)) return 15;
  slicer_free_snapshot(snapshot);
  if (!slicer_mesh_snapshot_euler(mesh, 1, 0, 0, 0, 1, &snapshot)) return 16;
  vertex_data = slicer_snapshot_vertices(snapshot, &vertex_count);
  vertices.assign(vertex_data, vertex_data + vertex_count);
  if (!has_vertex(vertices, 40, 0, 0)) return 17;
  slicer_free_snapshot(snapshot);

  const std::array<std::array<double,3>,3> independent = {{{90,0,0},{0,90,0},{0,0,90}}};
  std::array<slicer_bounds_t,3> independent_bounds{};
  for (size_t i = 0; i < independent.size(); ++i) {
     if (!slicer_mesh_snapshot_euler(mesh, 4, independent[i][0], independent[i][1], independent[i][2], 1, &snapshot) ||
         !slicer_snapshot_bounds(snapshot, &independent_bounds[i]) || !finite_bounds(independent_bounds[i])) return 5;
    slicer_free_snapshot(snapshot);
  }
  if (close(independent_bounds[0].max_y, 30) || close(independent_bounds[1].max_x, 20) ||
      close(independent_bounds[2].max_x, 20)) return 6;

  if (!slicer_config_set_transform_euler(config, 4, 31, -17, 43, 1.25)) return 7;
  slicer_result_t combined = slicer_slice(mesh, config);
  if (!combined || slicer_result_layer_count(combined) <= 8) return 8;
  if (!slicer_mesh_snapshot_euler(mesh, 4, 31, -17, 43, 1.25, &snapshot)) return 9;
  slicer_bounds_t combined_bounds{};
  if (!slicer_snapshot_bounds(snapshot, &combined_bounds) || !finite_bounds(combined_bounds)) return 10;
  if (combined_bounds.max_z <= 40) return 11;
  slicer_free_snapshot(snapshot);

  const std::array<std::pair<int,double>,4> invalid = {{{-1,0},{6,0},{4,181},{4,-181}}};
  for (auto [axis, angle] : invalid) if (slicer_config_set_transform_euler(config, axis, angle, 0, 0, 1)) return 12;
  if (slicer_config_set_transform_euler(config, 4, 0, 0, 0, 0) ||
      slicer_config_set_transform_euler(config, 4, NAN, 0, 0, 1) ||
      slicer_config_set_transform_euler(config, 4, 0, INFINITY, 0, 1) ||
      slicer_config_set_transform_euler(config, 4, 0, 0, -181, 1) ||
      slicer_config_set_transform_euler(config, 4, 0, 0, 0, INFINITY) ||
      slicer_mesh_snapshot_euler(mesh, 4, 0, 0, 181, 1, &snapshot) ||
      slicer_last_error()[0] == '\0') return 13;

  slicer_free_result(combined); slicer_free_result(identity); slicer_free_config(config); slicer_free_mesh(mesh);
  std::cout << "transform regression: PASS\n";
  return 0;
}
