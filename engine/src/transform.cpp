#include "transform.h"

#include <algorithm>
#include <cmath>

namespace layer_cut {
namespace {

Vec3 rotate_x(Vec3 p, double radians) {
  const double c = std::cos(radians), s = std::sin(radians);
  return {p.x, static_cast<float>(p.y * c - p.z * s),
          static_cast<float>(p.y * s + p.z * c)};
}

Vec3 rotate_y(Vec3 p, double radians) {
  const double c = std::cos(radians), s = std::sin(radians);
  return {static_cast<float>(p.x * c + p.z * s), p.y,
          static_cast<float>(-p.x * s + p.z * c)};
}

Vec3 rotate_z(Vec3 p, double radians) {
  const double c = std::cos(radians), s = std::sin(radians);
  return {static_cast<float>(p.x * c - p.y * s),
          static_cast<float>(p.x * s + p.y * c), p.z};
}

Vec3 orient(Vec3 p, int axis) {
  switch (axis) {
    // Preserve the selected signed model axis as slicer +Z.
    case 0: return {-p.z, p.y, p.x};
    case 1: return {p.z, p.y, -p.x};
    case 2: return {p.x, -p.z, p.y};
    case 3: return {p.x, p.z, -p.y};
    case 5: return {p.x, -p.y, -p.z};
    default: return p;
  }
}

}  // namespace

Mesh transform_mesh(const Mesh& source, const TransformOptions& options) {
  Mesh output;
  const double degrees_to_radians = 3.14159265358979323846 / 180.0;
  const double x = options.rotate_x_degrees * degrees_to_radians;
  const double y = options.rotate_y_degrees * degrees_to_radians;
  const double z = options.rotate_z_degrees * degrees_to_radians;
  bool first = true;
  auto apply = [&](Vec3 point) {
    point = rotate_x(point, x);
    point = rotate_y(point, y);
    point = rotate_z(point, z);
    point = orient(point, options.cutting_axis);
    return Vec3{static_cast<float>(point.x * options.scale),
                static_cast<float>(point.y * options.scale),
                static_cast<float>(point.z * options.scale)};
  };
  for (const auto& triangle : source.triangles) {
    const Triangle copy{triangle.normal, apply(triangle.a), apply(triangle.b),
                        apply(triangle.c)};
    output.triangles.push_back(copy);
    for (const auto& point : {copy.a, copy.b, copy.c}) {
      if (first) {
        output.min = output.max = point;
        first = false;
      } else {
        output.min.x = std::min(output.min.x, point.x);
        output.min.y = std::min(output.min.y, point.y);
        output.min.z = std::min(output.min.z, point.z);
        output.max.x = std::max(output.max.x, point.x);
        output.max.y = std::max(output.max.y, point.y);
        output.max.z = std::max(output.max.z, point.z);
      }
    }
  }
  if (!first) {
    const float offset = output.min.z;
    for (auto& triangle : output.triangles) {
      for (auto* point : {&triangle.a, &triangle.b, &triangle.c}) point->z -= offset;
    }
    output.min.z = 0.0f;
    output.max.z -= offset;
  }
  return output;
}

}  // namespace layer_cut
