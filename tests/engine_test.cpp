#include "engine.h"
#include "stl_loader.h"

#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

// Helper: create a valid binary STL file from triangles
static void create_stl(const std::string& path,
                       const std::vector<layer_cut::Triangle>& triangles) {
  std::string header("layer-cut binary STL", 20);
  header.resize(80, '\0');

  uint32_t count = static_cast<uint32_t>(triangles.size());
  std::string data;
  data.resize(80 + 4 + triangles.size() * 50, '\0');

  std::memcpy(&data[80], &count, 4);

  for (size_t i = 0; i < triangles.size(); ++i) {
    uint32_t offset = 84 + i * 50;
    std::memcpy(&data[offset], &triangles[i].normal, sizeof(layer_cut::Vec3));
    std::memcpy(&data[offset + 12], &triangles[i].a, sizeof(layer_cut::Vec3));
    std::memcpy(&data[offset + 24], &triangles[i].b, sizeof(layer_cut::Vec3));
    std::memcpy(&data[offset + 36], &triangles[i].c, sizeof(layer_cut::Vec3));
    // 2-byte attribute byte count (zero)
    std::memset(&data[offset + 48], 0, 2);
  }

  std::ofstream file(path, std::ios::binary);
  file.write(data.c_str(), data.size());
}

// Helper: create a truncated binary STL (valid header + count, but truncated
// triangle data)
static void create_truncated_stl(const std::string& path,
                                 uint32_t claimed_triangles,
                                 uint32_t actual_triangles) {
  std::string header("layer-cut truncated STL", 23);
  header.resize(80, '\0');

  std::string data;
  // Write header + count + partial triangles (truncated)
  size_t size = 84 + actual_triangles * 50;
  data.resize(size, '\0');

  std::memcpy(&data[80], &claimed_triangles, 4);

  for (size_t i = 0; i < actual_triangles; ++i) {
    uint32_t offset = 84 + i * 50;
    std::memset(&data[offset], 0, 50);
  }

  std::ofstream file(path, std::ios::binary);
  file.write(data.c_str(), data.size());
}

// Helper: create an ASCII STL file
static void create_ascii_stl(const std::string& path) {
  std::string ascii =
      "solid test\n"
      "  facet normal 0 0 1\n"
      "    outer loop\n"
      "      vertex 0 0 0\n"
      "      vertex 1 0 0\n"
      "      vertex 0 1 0\n"
      "    endloop\n"
      "  endfacet\n"
      "endsolid test\n";

  std::ofstream file(path, std::ios::binary);
  file.write(ascii.c_str(), ascii.size());
}

// Helper: create a file starting with "solid" but with binary garbage
// (to test ASCII detection)
static void create_fake_ascii_stl(const std::string& path) {
  std::string header = "solid";
  header.resize(80, '\0');

  std::string data = header;
  uint32_t count = 1;
  data.append(reinterpret_cast<const char*>(&count), 4);

  // Add binary triangle data (not valid ASCII)
  std::string triangle(50, '\0');
  data.append(triangle);

  std::ofstream file(path, std::ios::binary);
  file.write(data.c_str(), data.size());
}

int main() {
  int failures = 0;

  // Bootstrap check
  if (layer_cut::engine_version_major() != 0) {
    failures++;
  }

  // Test 1: Valid STL with one triangle
  {
    layer_cut::Triangle tri;
    tri.normal = {0, 0, 1};
    tri.a = {0, 0, 0};
    tri.b = {1, 0, 0};
    tri.c = {0, 1, 0};

    create_stl("/tmp/test_valid.stl", {tri});

    layer_cut::LoadError error;
    auto result =
        layer_cut::load_stl("/tmp/test_valid.stl", layer_cut::LoadLimits(),
                            &error);

    if (result.size() != 1) {
      failures++;
    }
  }

  // Test 2: Valid STL with multiple triangles
  {
    std::vector<layer_cut::Triangle> triangles;
    triangles.push_back({{0, 0, 1}, {0, 0, 0}, {1, 0, 0}, {0, 1, 0}});
    triangles.push_back({{0, 0, 1}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}});

    create_stl("/tmp/test_multi.stl", triangles);

    layer_cut::LoadError error;
    auto result = layer_cut::load_stl("/tmp/test_multi.stl",
                                      layer_cut::LoadLimits(), &error);

    if (result.size() != 2) {
      failures++;
    }
  }

  // Test 3: File not found
  {
    layer_cut::LoadError error;
    auto result = layer_cut::load_stl("/tmp/nonexistent_file.stl",
                                      layer_cut::LoadLimits(), &error);

    if (!result.empty() || error.code != layer_cut::LoadError::Code::FILE_NOT_FOUND) {
      failures++;
    }
  }

  // Test 4: Truncated file (claims 10 triangles, only has 1)
  {
    create_truncated_stl("/tmp/test_truncated.stl", 10, 1);

    layer_cut::LoadError error;
    auto result = layer_cut::load_stl("/tmp/test_truncated.stl",
                                      layer_cut::LoadLimits(), &error);

    if (!result.empty() ||
        error.code != layer_cut::LoadError::Code::TRUNCATED) {
      failures++;
    }
  }

  // Test 5: File too many triangles
  {
    layer_cut::Triangle tri;
    tri.normal = {0, 0, 1};
    tri.a = {0, 0, 0};
    tri.b = {1, 0, 0};
    tri.c = {0, 1, 0};

    create_stl("/tmp/test_big.stl", {tri});

    layer_cut::LoadLimits limits;
    limits.max_header_triangles = 0;  // Zero limit

    layer_cut::LoadError error;
    auto result = layer_cut::load_stl("/tmp/test_big.stl", limits, &error);

    if (!result.empty() ||
        error.code != layer_cut::LoadError::Code::TOO_MANY_TRIANGLES) {
      failures++;
    }
  }

  // Test 6: Non-finite coordinates
  {
    std::string data;
    data.resize(84 + 50, '\0');
    std::string header("layer-cut nonfinite STL", 23);
    header.resize(80, '\0');
    data.replace(0, 80, header);

    uint32_t count = 1;
    std::memcpy(&data[80], &count, 4);

    // Write a triangle with NaN coordinates
    float nan_val = std::numeric_limits<float>::quiet_NaN();
    std::memcpy(&data[84], &nan_val, 4);  // normal.x = NaN
    std::memcpy(&data[96], &nan_val, 4);  // a.x = NaN
    std::memcpy(&data[108], &nan_val, 4);  // b.x = NaN
    std::memcpy(&data[120], &nan_val, 4);  // c.x = NaN

    std::ofstream file("/tmp/test_nan.stl", std::ios::binary);
    file.write(data.c_str(), data.size());
    file.close();

    layer_cut::LoadError error;
    auto result = layer_cut::load_stl("/tmp/test_nan.stl",
                                      layer_cut::LoadLimits(), &error);

    if (!result.empty() ||
        error.code != layer_cut::LoadError::Code::NONFINITE_COORDINATE) {
      failures++;
    }
  }

  // Test 7: ASCII STL detection (file starting with "solid")
  {
    create_ascii_stl("/tmp/test_ascii.stl");

    layer_cut::LoadError error;
    auto result = layer_cut::load_stl("/tmp/test_ascii.stl",
                                      layer_cut::LoadLimits(), &error);

    if (!result.empty() ||
        error.code != layer_cut::LoadError::Code::NOT_BINARY_STL) {
      failures++;
    }
  }

  // Test 8: Binary STL whose header starts with "solid"
  {
    create_fake_ascii_stl("/tmp/test_fake_ascii.stl");

    layer_cut::LoadError error;
    auto result = layer_cut::load_stl("/tmp/test_fake_ascii.stl",
                                      layer_cut::LoadLimits(), &error);

    if (result.size() != 1 || error.code != layer_cut::LoadError::Code::OK) {
      failures++;
    }
  }

  // Test 9: File too small (less than header + triangle)
  {
    std::string data = "solid";
    data.resize(50, '\0');  // Less than 84 bytes

    std::ofstream file("/tmp/test_small.stl", std::ios::binary);
    file.write(data.c_str(), data.size());
    file.close();

    layer_cut::LoadError error;
    auto result = layer_cut::load_stl("/tmp/test_small.stl",
                                      layer_cut::LoadLimits(), &error);

    if (!result.empty() ||
        error.code != layer_cut::LoadError::Code::NOT_BINARY_STL) {
      failures++;
    }
  }

  // Test 10: Custom limits (max file size)
  {
    layer_cut::Triangle tri;
    tri.normal = {0, 0, 1};
    tri.a = {0, 0, 0};
    tri.b = {1, 0, 0};
    tri.c = {0, 1, 0};

    create_stl("/tmp/test_size_limit.stl", {tri});

    layer_cut::LoadLimits limits;
    limits.max_file_size = 10;  // Very small limit

    layer_cut::LoadError error;
    auto result = layer_cut::load_stl("/tmp/test_size_limit.stl", limits,
                                      &error);

    if (!result.empty() ||
        error.code != layer_cut::LoadError::Code::INVALID_FORMAT) {
      failures++;
    }
  }

  // Test 11: Valid STL with no triangles (empty mesh)
  {
    std::string header("layer-cut empty STL", 19);
    header.resize(80, '\0');

    std::string data;
    data.resize(84, '\0');
    data.replace(0, 80, header);

    uint32_t count = 0;
    std::memcpy(&data[80], &count, 4);

    std::ofstream file("/tmp/test_empty.stl", std::ios::binary);
    file.write(data.c_str(), data.size());
    file.close();

    layer_cut::LoadError error;
    auto result = layer_cut::load_stl("/tmp/test_empty.stl",
                                      layer_cut::LoadLimits(), &error);

    if (!result.empty() || error.code != layer_cut::LoadError::Code::OK) {
      failures++;
    }
  }

  return failures;
}
