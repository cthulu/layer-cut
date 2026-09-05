#pragma once

#include "mesh.h"

#include <string>
#include <vector>

namespace layer_cut {

// Serialize a non-empty triangle collection as an ASCII STL document.
// Coordinates are emitted unchanged and are interpreted as millimetres.
std::string make_ascii_stl(const std::vector<Triangle>& triangles,
                           const std::string& solid_name = "layer_cut",
                           std::string* error = nullptr);

// Write an ASCII STL document to disk. Returns false and fills error on failure.
bool write_ascii_stl_file(const std::string& path,
                          const std::vector<Triangle>& triangles,
                          const std::string& solid_name = "layer_cut",
                          std::string* error = nullptr);

}  // namespace layer_cut
