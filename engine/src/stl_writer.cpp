#include "stl_writer.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>

namespace layer_cut {
namespace {

bool finite(const Vec3& point) {
  return std::isfinite(point.x) && std::isfinite(point.y) &&
         std::isfinite(point.z);
}

Vec3 cross(const Vec3& a, const Vec3& b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
          a.x * b.y - a.y * b.x};
}

Vec3 subtract(const Vec3& a, const Vec3& b) {
  return {a.x - b.x, a.y - b.y, a.z - b.z};
}

bool normal_for(const Triangle& triangle, Vec3* normal) {
  const Vec3 computed = cross(subtract(triangle.b, triangle.a),
                              subtract(triangle.c, triangle.a));
  const double length = std::sqrt(static_cast<double>(computed.x) * computed.x +
                                  static_cast<double>(computed.y) * computed.y +
                                  static_cast<double>(computed.z) * computed.z);
  if (!std::isfinite(length) || length <= 0.0) return false;
  *normal = {static_cast<float>(computed.x / length),
             static_cast<float>(computed.y / length),
             static_cast<float>(computed.z / length)};
  return true;
}

bool valid_triangle(const Triangle& triangle) {
  return finite(triangle.a) && finite(triangle.b) && finite(triangle.c);
}

}  // namespace

std::string make_ascii_stl(const std::vector<Triangle>& triangles,
                           const std::string& solid_name,
                           std::string* error) {
  if (error) error->clear();
  if (triangles.empty()) {
    if (error) *error = "Cannot write an empty STL mesh";
    return {};
  }

  std::ostringstream output;
  output << std::fixed << std::setprecision(9);
  output << "solid " << (solid_name.empty() ? "layer_cut" : solid_name) << '\n';
  for (std::size_t index = 0; index < triangles.size(); ++index) {
    const Triangle& triangle = triangles[index];
    if (!valid_triangle(triangle)) {
      if (error) {
        *error = "Triangle " + std::to_string(index) +
                 " contains a non-finite coordinate";
      }
      return {};
    }
    Vec3 normal;
    if (!normal_for(triangle, &normal)) {
      if (error) {
        *error = "Triangle " + std::to_string(index) +
                 " is degenerate";
      }
      return {};
    }
    output << "  facet normal " << normal.x << ' ' << normal.y << ' '
           << normal.z << "\n"
           << "    outer loop\n"
           << "      vertex " << triangle.a.x << ' ' << triangle.a.y << ' '
           << triangle.a.z << "\n"
           << "      vertex " << triangle.b.x << ' ' << triangle.b.y << ' '
           << triangle.b.z << "\n"
           << "      vertex " << triangle.c.x << ' ' << triangle.c.y << ' '
           << triangle.c.z << "\n"
           << "    endloop\n"
           << "  endfacet\n";
  }
  output << "endsolid " << (solid_name.empty() ? "layer_cut" : solid_name)
         << '\n';
  return output.str();
}

std::string make_binary_stl(const std::vector<Triangle>& triangles,
                            const std::string& solid_name,
                            std::string* error) {
  if (error) error->clear();
  if (triangles.empty()) {
    if (error) *error = "Cannot write an empty STL mesh";
    return {};
  }
  if (triangles.size() > std::numeric_limits<uint32_t>::max()) {
    if (error) *error = "Too many triangles for binary STL";
    return {};
  }

  std::string output(84 + triangles.size() * 50, '\0');
  const std::string header = "layer-cut binary STL " + solid_name;
  std::memcpy(output.data(), header.data(), std::min(header.size(), size_t{80}));
  const uint32_t count = static_cast<uint32_t>(triangles.size());
  std::memcpy(output.data() + 80, &count, sizeof(count));
  for (std::size_t i = 0; i < triangles.size(); ++i) {
    const Triangle& triangle = triangles[i];
    if (!valid_triangle(triangle)) {
      if (error) *error = "Triangle " + std::to_string(i) + " contains a non-finite coordinate";
      return {};
    }
    Vec3 normal;
    if (!normal_for(triangle, &normal)) {
      if (error) *error = "Triangle " + std::to_string(i) + " is degenerate";
      return {};
    }
    char* record = output.data() + 84 + i * 50;
    std::memcpy(record, &normal, sizeof(normal));
    std::memcpy(record + 12, &triangle.a, sizeof(Vec3));
    std::memcpy(record + 24, &triangle.b, sizeof(Vec3));
    std::memcpy(record + 36, &triangle.c, sizeof(Vec3));
  }
  return output;
}

bool write_ascii_stl_file(const std::string& path,
                          const std::vector<Triangle>& triangles,
                          const std::string& solid_name,
                          std::string* error) {
  std::string serialization_error;
  const std::string document =
      make_ascii_stl(triangles, solid_name, &serialization_error);
  if (document.empty()) {
    if (error) *error = serialization_error;
    return false;
  }
  std::ofstream file(path, std::ios::binary);
  if (!file) {
    if (error) *error = "Cannot open STL output: " + path;
    return false;
  }
  file << document;
  if (!file) {
    if (error) *error = "Cannot write STL output: " + path;
    return false;
  }
  return true;
}

}  // namespace layer_cut
