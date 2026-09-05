#pragma once

#include "slicer.h"

#include <cstddef>
#include <string>
#include <vector>

namespace layer_cut {

enum class CricutPageSize { NORMAL, LARGE };

struct CricutPageOptions {
  CricutPageSize size = CricutPageSize::NORMAL;
  double gap_mm = 3.0;
  std::size_t path_limit = 4500;
};

struct CricutTile {
  std::size_t layer_index = 0;
  double z = 0.0;
  double origin_x = 0.0;
  double origin_y = 0.0;
  std::vector<Contour> contours;
};

struct CricutPage {
  std::size_t page_index = 0;
  double width_mm = 0.0;
  double height_mm = 0.0;
  double tile_width_mm = 0.0;
  double tile_height_mm = 0.0;
  std::vector<CricutTile> tiles;
  std::size_t cut_path_count = 0;
};

struct CricutPageResult {
  std::vector<CricutPage> pages;
  std::vector<std::string> warnings;
  std::string error;

  bool ok() const { return error.empty(); }
};

CricutPageResult build_cricut_pages(
    const std::vector<SliceLayer>& layers,
    const CricutPageOptions& options = CricutPageOptions());

std::string make_cricut_cut_svg(const CricutPage& page);
std::string make_cricut_guide_svg(const CricutPage& page,
                                  double inset_mm = 1.0,
                                  std::vector<std::string>* warnings = nullptr);
std::string make_cricut_combined_svg(const CricutPage& page,
                                     double inset_mm = 1.0,
                                     std::vector<std::string>* warnings = nullptr);

}  // namespace layer_cut
