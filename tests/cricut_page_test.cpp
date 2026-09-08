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

layer_cut::SliceLayer rectangle_layer(std::size_t index, double z, double width,
                                      double height) {
  layer_cut::SliceLayer value;
  value.index = index;
  value.z = z;
  value.contours.push_back({{{0, 0}, {width, 0}, {width, height}, {0, height}},
                            false, width * height});
  return value;
}

layer_cut::SliceLayer complex_layer(std::size_t index, double z) {
  layer_cut::SliceLayer value = rectangle_layer(index, z, 300, 100);
  value.contours.push_back({{{20, 20}, {40, 20}, {40, 40}, {20, 40}}, true,
                            -400.0});
  value.contours.push_back({{{250, 10}, {280, 10}, {280, 30}, {250, 30}}, false,
                            600.0});
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
  if (combined.find("layer-numbers") != std::string::npos ||
      cut.find("layer-numbers") != std::string::npos ||
      guide.find("layer-numbers") != std::string::npos) {
    return 21;
  }

  auto numbered_negative = rectangle_layer(0, 0.5, 20.0, 20.0);
  numbered_negative.contours[0].points = {{-20, -10}, {0, -10}, {0, 10}, {-20, 10}};
  numbered_negative.contours.push_back({{{-15, -5}, {-12, -5}, {-12, -2}, {-15, -2}},
                                        true, -9.0});
  numbered_negative.contours.push_back({{{5, 5}, {10, 5}, {10, 8}, {5, 8}}, false,
                                        15.0});
  layer_cut::SliceLayer empty;
  empty.index = 8;
  empty.z = 8.5;
  const auto tiny = rectangle_layer(7, 7.5, 0.5, 0.5);
  layer_cut::CricutPageOptions numbered_options = options;
  numbered_options.show_layer_numbers = true;
  const auto numbered = layer_cut::build_cricut_pages(
      {numbered_negative, empty, tiny}, numbered_options);
  if (!numbered.ok() || numbered.pages.size() != 1 ||
      numbered.pages.front().tiles.size() != 2) {
    return 22;
  }
  const std::string numbered_combined =
      layer_cut::make_cricut_combined_svg(numbered.pages.front(), 1.0);
  const std::string numbered_cut =
      layer_cut::make_cricut_cut_svg(numbered.pages.front());
  if (numbered_combined.find("<g id=\"layer-numbers\" data-operation=\"layer-number\">") ==
          std::string::npos ||
      numbered_combined.find("id=\"layer-number-000\"") == std::string::npos ||
      numbered_combined.find("id=\"layer-number-007\"") == std::string::npos ||
      numbered_combined.find("data-layer-index=\"8\"") != std::string::npos ||
      numbered_combined.find("font-size=\"2.500000000mm\"") == std::string::npos ||
      numbered_combined.find("fill=\"green\"") == std::string::npos ||
      numbered_combined.find("font-family=\"sans-serif\"") == std::string::npos ||
      numbered_combined.find("text-anchor=\"middle\"") == std::string::npos ||
      numbered_combined.find("dominant-baseline=\"middle\"") == std::string::npos ||
      numbered_combined.find("<title>LAYER NUMBER | Layer 007 | 8</title>") ==
          std::string::npos ||
      numbered_combined.find("\n      8\n") == std::string::npos ||
      numbered_combined.find("x=\"22.000000000\" y=\"17.000000000\"") ==
          std::string::npos ||
      numbered_cut.find("layer-numbers") != std::string::npos) {
    return 23;
  }
  if (numbered_combined != layer_cut::make_cricut_combined_svg(
                               numbered.pages.front(), 1.0) ||
      numbered_combined.find("data-operation=\"cut\"") == std::string::npos ||
      numbered_combined.find("data-operation=\"draw\"") == std::string::npos) {
    return 24;
  }
  numbered_options.size = layer_cut::CricutPageSize::LARGE;
  numbered_options.packing = layer_cut::CricutPackingStrategy::TIGHT;
  const auto numbered_rotated = layer_cut::build_cricut_pages(
      {rectangle_layer(0, 0.5, 10, 10), rectangle_layer(7, 1.5, 300, 100)},
      numbered_options);
  if (!numbered_rotated.ok() || numbered_rotated.pages.size() != 1 ||
      numbered_rotated.pages.front().tiles[1].orientation_degrees != 90 ||
      layer_cut::make_cricut_combined_svg(numbered_rotated.pages.front(), 1.0)
              .find("id=\"layer-number-007\"") == std::string::npos ||
      layer_cut::make_cricut_combined_svg(numbered_rotated.pages.front(), 1.0)
              .find("x=\"70.000000000\" y=\"157.000000000\"") ==
          std::string::npos) {
    return 25;
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

  options.size = layer_cut::CricutPageSize::NORMAL;
  options.gap_mm = 3.0;
  options.packing = layer_cut::CricutPackingStrategy::FIXED;
  const auto fixed = layer_cut::build_cricut_pages(
      {rectangle_layer(0, 0.5, 160, 100), rectangle_layer(1, 1.5, 100, 160)},
      options);
  options.packing = layer_cut::CricutPackingStrategy::TIGHT;
  const auto tight = layer_cut::build_cricut_pages(
      {rectangle_layer(0, 0.5, 160, 100), rectangle_layer(1, 1.5, 100, 160)},
      options);
  if (!fixed.ok() || !tight.ok() || fixed.pages.size() <= tight.pages.size() ||
      tight.pages.size() != 1 || tight.pages.front().tiles.size() != 2) return 12;
  const auto fixed_repeat = layer_cut::build_cricut_pages(
      {rectangle_layer(0, 0.5, 160, 100), rectangle_layer(1, 1.5, 100, 160)},
      layer_cut::CricutPageOptions{options.size,
                                   layer_cut::CricutPackingStrategy::FIXED,
                                   options.gap_mm, options.path_limit});
  if (layer_cut::make_cricut_cut_svg(fixed.pages.front()) !=
          layer_cut::make_cricut_cut_svg(fixed_repeat.pages.front()) ||
      layer_cut::make_cricut_cut_svg(fixed.pages.front()).find("data-orientation") !=
          std::string::npos) return 19;
  const std::string tight_cut = layer_cut::make_cricut_cut_svg(tight.pages.front());
  if (tight_cut.find("data-orientation=\"0\"") == std::string::npos ||
      tight_cut.find("data-bounds=\"") == std::string::npos) return 13;
  const auto tight_repeat = layer_cut::build_cricut_pages(
      {rectangle_layer(0, 0.5, 160, 100), rectangle_layer(1, 1.5, 100, 160)},
      options);
  if (layer_cut::make_cricut_cut_svg(tight.pages.front()) !=
      layer_cut::make_cricut_cut_svg(tight_repeat.pages.front())) return 14;

  options.size = layer_cut::CricutPageSize::LARGE;
  const auto rotated = layer_cut::build_cricut_pages(
      {rectangle_layer(0, 0.1, 10, 10), rectangle_layer(7, 0.5, 300, 100)}, options);
  if (!rotated.ok() || rotated.pages.front().tiles.size() != 2 ||
      rotated.pages.front().tiles[1].orientation_degrees != 90 ||
      rotated.pages.front().tiles[1].width_mm != 100.0 ||
      rotated.pages.front().tiles[1].height_mm != 300.0) return 15;
  const std::string rotated_guide =
      layer_cut::make_cricut_guide_svg(rotated.pages.front(), 1.0);
  if (rotated_guide.find("data-orientation=\"90\"") == std::string::npos) return 16;

  const auto complex = layer_cut::build_cricut_pages({complex_layer(3, 0.5)}, options);
  if (!complex.ok() || complex.pages.front().tiles.front().contours.size() != 3 ||
      !complex.pages.front().tiles.front().contours[1].hole ||
      complex.pages.front().tiles.front().contours[1].signed_area != -400.0) return 20;

  options.gap_mm = -1.0;
  const auto tight_overlap = layer_cut::build_cricut_pages(
      {rectangle_layer(0, 0.5, 20, 20), rectangle_layer(1, 1.5, 20, 20)}, options);
  if (!tight_overlap.ok() || tight_overlap.warnings.empty()) return 17;
  options.gap_mm = 3.0;
  options.path_limit = 2;
  if (layer_cut::build_cricut_pages(
          {rectangle_layer(0, 0.5, 20, 20), rectangle_layer(1, 1.5, 20, 20)},
          options)
          .ok()) return 18;
  return 0;
}
