#include "cricut_page.h"

#include <string>

namespace {

layer_cut::SliceLayer layer(std::size_t index, double z) {
  layer_cut::SliceLayer value;
  value.index = index;
  value.z = z;
  value.contours.push_back({{{0, 0}, {10, 0}, {10, 10}, {0, 10}}, false,
                            100.0});
  return value;
}

layer_cut::SliceLayer large_layer(std::size_t index, double z) {
  layer_cut::SliceLayer value;
  value.index = index;
  value.z = z;
  value.contours.push_back({{{0, 0}, {100, 0}, {100, 100}, {0, 100}}, false,
                            10000.0});
  return value;
}

layer_cut::SliceLayer rectangle_layer(std::size_t index, double z, double size) {
  layer_cut::SliceLayer value;
  value.index = index;
  value.z = z;
  value.contours.push_back({{{0, 0}, {size, 0}, {size, size}, {0, size}}, false,
                            size * size});
  return value;
}

}  // namespace

int main() {
  layer_cut::CricutPageOptions options;
  options.size = layer_cut::CricutPageSize::NORMAL;
  options.gap_mm = 3.0;
  const auto result = layer_cut::build_cricut_pages(
      {layer(0, 0.5), layer(1, 1.5), layer(2, 2.5)}, options);
  if (!result.ok() || result.pages.size() != 1 ||
      result.pages.front().tiles.size() != 3 ||
      result.pages.front().cut_path_count != 4) {
    return 1;
  }

  const auto& page = result.pages.front();
  if (page.width_mm != 304.0 || page.height_mm != 304.0 ||
      page.tiles[0].layer_index != 0 || page.tiles[1].layer_index != 1 ||
      page.tiles[2].layer_index != 2) {
    return 2;
  }
  const std::string cut = layer_cut::make_cricut_cut_svg(page);
  const std::string guide = layer_cut::make_cricut_guide_svg(page, 1.0);
  const std::string marker =
      "M 1.500000000 3.500000000 L 5.500000000 3.500000000";
  if (cut.find(marker) == std::string::npos ||
      guide.find(marker) == std::string::npos ||
      cut.find("CUT | Layer 000") == std::string::npos ||
      guide.find("PEN GUIDE | Layer 001 over Layer 000") == std::string::npos) {
    return 3;
  }
  const std::string combined = layer_cut::make_cricut_combined_svg(page, 1.0);
  if (combined.find("id=\"alignment-marker\"") != std::string::npos ||
      combined.find("id=\"cut-layers\"") == std::string::npos ||
      combined.find("id=\"pen-layers\"") == std::string::npos ||
      combined.find("data-operation=\"cut\"") == std::string::npos ||
      combined.find("data-operation=\"draw\"") == std::string::npos) {
    return 4;
  }

  const auto outside_result = layer_cut::build_cricut_pages(
      {rectangle_layer(0, 0.5, 8.0), rectangle_layer(1, 1.5, 10.0)}, options);
  if (!outside_result.ok()) return 10;
  std::vector<std::string> outside_warnings;
  const auto outside_guide = layer_cut::make_cricut_guide_svg(
      outside_result.pages.front(), 1.0, &outside_warnings);
  const auto outside_combined = layer_cut::make_cricut_combined_svg(
      outside_result.pages.front(), 1.0, &outside_warnings);
  if (outside_warnings.empty() ||
      outside_guide.find("id=\"pen-guide-001-over-000\"") == std::string::npos ||
      outside_guide.find("M 7.000000000 7.000000000") == std::string::npos ||
      outside_combined.find("id=\"pen-guide-001-over-000\"") == std::string::npos ||
      outside_combined.find("M 7.000000000 7.000000000") == std::string::npos) {
    return 11;
  }

  options.gap_mm = 3.0;
  options.size = layer_cut::CricutPageSize::NORMAL;
  const auto split = layer_cut::build_cricut_pages(
      {large_layer(0, 0.5), large_layer(1, 1.5), large_layer(2, 2.5),
       large_layer(3, 3.5), large_layer(4, 4.5)},
      options);
  if (!split.ok() || split.pages.size() != 2 ||
      split.pages[0].tiles.size() != 4 || split.pages[1].tiles.size() != 1) {
    return 5;
  }

  options.gap_mm = -11.0;
  const auto invalid = layer_cut::build_cricut_pages({layer(0, 0.5)}, options);
  if (invalid.ok()) return 6;

  options.gap_mm = -1.0;
  const auto overlapping = layer_cut::build_cricut_pages(
      {layer(0, 0.5), layer(1, 1.5)}, options);
  if (!overlapping.ok() || overlapping.warnings.empty()) return 7;

  options.gap_mm = 3.0;
  options.size = layer_cut::CricutPageSize::LARGE;
  const auto large = layer_cut::build_cricut_pages({layer(0, 0.5)}, options);
  if (!large.ok() || large.pages.front().height_mm != 608.0) return 8;

  layer_cut::SliceLayer oversized;
  oversized.index = 0;
  oversized.z = 0.5;
  oversized.contours.push_back({{{0, 0}, {291, 0}, {291, 1}, {0, 1}}, false,
                                291.0});
  if (layer_cut::build_cricut_pages({oversized}, options).ok()) return 9;
  return 0;
}
