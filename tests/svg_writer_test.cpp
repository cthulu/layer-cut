#include "svg_writer.h"

#include <string>

int main() {
  const layer_cut::Contour square = {
      {{1, 2}, {5, 2}, {5, 6}, {1, 6}}, false, 16.0};
  layer_cut::SvgOptions options;
  options.min = {1, 2};
  options.max = {5, 6};
  options.layer_index = 7;
  options.layer_z = 1.5;
  const std::string svg = layer_cut::make_svg({square}, options);
  if (svg.find("viewBox=\"1.000000000 2.000000000 4.000000000 4.000000000\"") ==
          std::string::npos ||
      svg.find("width=\"4.000000000mm\"") == std::string::npos ||
      svg.find("fill-rule=\"evenodd\"") == std::string::npos ||
      svg.find("stroke=\"none\"") == std::string::npos ||
      svg.find("M 1.000000000 2.000000000 L 5.000000000 2.000000000") ==
          std::string::npos ||
      svg.find("Z\"/>") == std::string::npos) {
    return 1;
  }
  return 0;
}
