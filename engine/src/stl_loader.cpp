#include "stl_loader.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>

namespace layer_cut {

namespace {

constexpr uint32_t kTriangleSize = 50;  // 12 floats (48 bytes) + 2-byte count
constexpr uint32_t kHeaderSize = 80;

bool starts_with_solid(const std::string& data) {
  if (data.size() < 5) return false;
  return std::strncmp(data.c_str(), "solid", 5) == 0;
}

bool has_finite_coordinates(const Vec3& v) {
  return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}

}  // namespace

std::vector<Triangle> load_stl(const std::string& path,
                               const LoadLimits& limits,
                               LoadError* out_error) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    LoadError err = LoadError::make(LoadError::Code::FILE_NOT_FOUND,
                                    "Cannot open file: " + path);
    if (out_error) *out_error = err;
    return {};
  }

  // Get file size
  file.seekg(0, std::ios::end);
  const std::streampos end = file.tellg();
  if (end < 0) {
    LoadError err = LoadError::make(LoadError::Code::INVALID_FORMAT,
                                    "Cannot determine file size: " + path);
    if (out_error) *out_error = err;
    return {};
  }
  const uint64_t file_size = static_cast<uint64_t>(end);
  file.seekg(0, std::ios::beg);

  if (file_size > limits.max_file_size) {
    LoadError err = LoadError::make(
        LoadError::Code::INVALID_FORMAT,
        "File exceeds maximum size: " + std::to_string(file_size) +
            " bytes (limit: " + std::to_string(limits.max_file_size) + ")");
    if (out_error) *out_error = err;
    return {};
  }

  // Read entire file
  std::string data(static_cast<size_t>(file_size), '\0');
  if (file_size != 0 && !file.read(data.data(), static_cast<std::streamsize>(file_size))) {
    LoadError err = LoadError::make(LoadError::Code::TRUNCATED,
                                    "Failed to read file: " + path);
    if (out_error) *out_error = err;
    return {};
  }

  // Validate the fixed header and count before classifying a "solid" header.
  // Binary STL headers are arbitrary bytes, so a valid binary file may begin
  // with the same five characters as an ASCII STL.
  if (file_size < kHeaderSize + 4) {
    const LoadError::Code code = starts_with_solid(data)
                                     ? LoadError::Code::NOT_BINARY_STL
                                     : LoadError::Code::TRUNCATED;
    LoadError err = LoadError::make(LoadError::Code::TRUNCATED,
                                    "File too small for a valid STL");
    err.code = code;
    if (out_error) *out_error = err;
    return {};
  }

  // Parse triangle count (uint32 at offset 80)
  uint32_t triangle_count;
  std::memcpy(&triangle_count, data.c_str() + kHeaderSize, 4);

  if (triangle_count > limits.max_header_triangles) {
    const LoadError::Code code = starts_with_solid(data)
                                     ? LoadError::Code::NOT_BINARY_STL
                                     : LoadError::Code::TOO_MANY_TRIANGLES;
    LoadError err = LoadError::make(
        code,
        "Triangle count exceeds limit: " + std::to_string(triangle_count) +
            " (limit: " + std::to_string(limits.max_header_triangles) + ")");
    if (out_error) *out_error = err;
    return {};
  }

  if (triangle_count > limits.max_triangles) {
    LoadError err = LoadError::make(
        LoadError::Code::TOO_MANY_TRIANGLES,
        "Triangle count exceeds memory limit: " + std::to_string(triangle_count) +
            " (limit: " + std::to_string(limits.max_triangles) + ")");
    if (out_error) *out_error = err;
    return {};
  }

  // Compute this in 64-bit arithmetic to prevent uint32 overflow.
  const uint64_t expected_size =
      static_cast<uint64_t>(kHeaderSize) + 4ULL +
      static_cast<uint64_t>(triangle_count) * kTriangleSize;
  if (file_size != expected_size) {
    const LoadError::Code code = starts_with_solid(data)
                                     ? LoadError::Code::NOT_BINARY_STL
                                     : LoadError::Code::TRUNCATED;
    LoadError err = LoadError::make(code,
                                    "File size mismatch: expected " +
                                        std::to_string(expected_size) +
                                        " bytes, got " + std::to_string(file_size));
    if (out_error) *out_error = err;
    return {};
  }

  // Parse triangles
  std::vector<Triangle> triangles;
  triangles.reserve(triangle_count);

  for (uint32_t i = 0; i < triangle_count; ++i) {
    const size_t offset = static_cast<size_t>(kHeaderSize) + 4ULL +
                          static_cast<size_t>(i) * kTriangleSize;

    Triangle tri;
    std::memcpy(&tri.normal, data.c_str() + offset, sizeof(Vec3));
    std::memcpy(&tri.a, data.c_str() + offset + sizeof(Vec3), sizeof(Vec3));
    std::memcpy(&tri.b, data.c_str() + offset + 2 * sizeof(Vec3), sizeof(Vec3));
    std::memcpy(&tri.c, data.c_str() + offset + 3 * sizeof(Vec3), sizeof(Vec3));

    // Validate finite coordinates
    if (!has_finite_coordinates(tri.normal) ||
        !has_finite_coordinates(tri.a) || !has_finite_coordinates(tri.b) ||
        !has_finite_coordinates(tri.c)) {
      LoadError err = LoadError::make(LoadError::Code::NONFINITE_COORDINATE,
                                      "Non-finite coordinate in triangle " +
                                          std::to_string(i));
      if (out_error) *out_error = err;
      return {};
    }

    triangles.push_back(tri);
  }

  if (out_error) *out_error = LoadError::make(LoadError::Code::OK, "");
  return triangles;
}

}  // namespace layer_cut
