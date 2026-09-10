#pragma once

#include "mesh.h"

namespace layer_cut {

struct TransformOptions {
  int cutting_axis = 4;
  double rotate_x_degrees = 0.0;
  double rotate_y_degrees = 0.0;
  double rotate_z_degrees = 0.0;
  double scale = 1.0;
};

// Applies scale * axis orientation * Rz * Ry * Rx and normalizes min-Z to zero.
Mesh transform_mesh(const Mesh& source, const TransformOptions& options);

}  // namespace layer_cut
