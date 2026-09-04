#include "svg_writer.h"

#include <fstream>
#include <iomanip>
#include <sstream>

namespace layer_cut {

std::string make_svg(const std::vector<Contour>& contours,
                     const SvgOptions& options) {
  const double width = options.max.x - options.min.x;
  const double height = options.max.y - options.min.y;
  std::ostringstream svg;
  svg << std::fixed << std::setprecision(9);
  svg << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\""
      << options.min.x << ' ' << options.min.y << ' ' << width << ' ' << height
      << "\" width=\"" << width << "mm\" height=\"" << height
      << "mm\" data-layer-index=\"" << options.layer_index
      << "\" data-layer-z=\"" << options.layer_z << "\">\n";
  svg << "  <g fill=\"black\" fill-rule=\"evenodd\" stroke=\"none\">\n";
  for (const Contour& contour : contours) {
    if (contour.points.size() < 3) continue;
    svg << "    <path d=\"M " << contour.points[0].x << ' '
        << contour.points[0].y;
    for (std::size_t i = 1; i < contour.points.size(); ++i) {
      svg << " L " << contour.points[i].x << ' ' << contour.points[i].y;
    }
    svg << " Z\"/>\n";
  }
  svg << "  </g>\n</svg>\n";
  return svg.str();
}

bool write_svg_file(const std::string& path, const std::vector<Contour>& contours,
                    const SvgOptions& options, std::string* error) {
  std::ofstream file(path, std::ios::binary);
  if (!file) {
    if (error) *error = "Cannot open SVG output: " + path;
    return false;
  }
  file << make_svg(contours, options);
  if (!file) {
    if (error) *error = "Cannot write SVG output: " + path;
    return false;
  }
  return true;
}

}  // namespace layer_cut
