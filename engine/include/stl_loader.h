#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace layer_cut {

struct Vec3 {
  float x, y, z;
};

struct Triangle {
  Vec3 normal;
  Vec3 a, b, c;
};

struct LoadError {
  enum class Code {
    OK,
    FILE_NOT_FOUND,
    NOT_BINARY_STL,
    TRUNCATED,
    TOO_MANY_TRIANGLES,
    NONFINITE_COORDINATE,
    INVALID_FORMAT,
  };

  Code code = Code::OK;
  std::string message;

  static LoadError make(Code code, std::string message) {
    LoadError err;
    err.code = code;
    err.message = std::move(message);
    return err;
  }
};

struct LoadLimits {
  // Maximum file size in bytes (default: 1 GiB)
  uint64_t max_file_size = 1024ULL * 1024 * 1024;
  // Maximum number of triangles (default: 100 million)
  uint32_t max_triangles = 100'000'000;
  // Maximum triangle count from file header (default: 100 million)
  uint32_t max_header_triangles = 100'000'000;
};

// Parse a binary STL file into a vector of triangles.
// Returns a LoadError if the file is not a valid binary STL, is truncated,
// exceeds limits, or contains non-finite coordinates.
// ASCII STL files are detected and reported as unsupported.
std::vector<Triangle> load_stl(const std::string& path,
                               const LoadLimits& limits = LoadLimits(),
                               LoadError* out_error = nullptr);

}  // namespace layer_cut
