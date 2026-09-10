#include "cricut_page.h"

#include "polygon_ops.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>

namespace layer_cut {
namespace {

struct Bounds {
  double min_x = 0.0;
  double min_y = 0.0;
  double max_x = 0.0;
  double max_y = 0.0;
  bool set = false;
};

struct TightLayer {
  const SliceLayer* layer = nullptr;
  Bounds bounds;
};

struct Shelf {
  double y = 0.0;
  double height = 0.0;
  double next_x = 0.0;
};

struct TightPageState {
  CricutPage page;
  std::vector<Shelf> shelves;
};

Bounds bounds_of(const std::vector<Contour>& contours) {
  Bounds bounds;
  for (const Contour& contour : contours) {
    for (const Vec2& point : contour.points) {
      if (!bounds.set) {
        bounds = {point.x, point.y, point.x, point.y, true};
      } else {
        bounds.min_x = std::min(bounds.min_x, point.x);
        bounds.min_y = std::min(bounds.min_y, point.y);
        bounds.max_x = std::max(bounds.max_x, point.x);
        bounds.max_y = std::max(bounds.max_y, point.y);
      }
    }
  }
  return bounds;
}

std::vector<Contour> rotate_and_normalize(const std::vector<Contour>& contours,
                                           const Bounds& source, int degrees,
                                           Bounds* normalized) {
  const double radians = static_cast<double>(degrees) *
                         3.14159265358979323846 / 180.0;
  const double c = std::cos(radians);
  const double s = std::sin(radians);
  const double cx = (source.min_x + source.max_x) * 0.5;
  const double cy = (source.min_y + source.max_y) * 0.5;
  std::vector<Contour> rotated = contours;
  Bounds rotated_bounds;
  for (Contour& contour : rotated) {
    for (Vec2& point : contour.points) {
      const double x = point.x - cx;
      const double y = point.y - cy;
      point.x = x * c - y * s + cx;
      point.y = x * s + y * c + cy;
      if (!rotated_bounds.set) {
        rotated_bounds = {point.x, point.y, point.x, point.y, true};
      } else {
        rotated_bounds.min_x = std::min(rotated_bounds.min_x, point.x);
        rotated_bounds.min_y = std::min(rotated_bounds.min_y, point.y);
        rotated_bounds.max_x = std::max(rotated_bounds.max_x, point.x);
        rotated_bounds.max_y = std::max(rotated_bounds.max_y, point.y);
      }
    }
  }
  if (!rotated_bounds.set) {
    *normalized = rotated_bounds;
    return rotated;
  }
  for (Contour& contour : rotated) {
    for (Vec2& point : contour.points) {
      point.x -= rotated_bounds.min_x;
      point.y -= rotated_bounds.min_y;
    }
  }
  rotated_bounds.max_x -= rotated_bounds.min_x;
  rotated_bounds.max_y -= rotated_bounds.min_y;
  rotated_bounds.min_x = rotated_bounds.min_y = 0.0;
  *normalized = rotated_bounds;
  return rotated;
}

// The pen outline uses the same complete contour set as the cut geometry.
// Keep its bounds explicit so guide changes cannot escape the footprint.
Bounds pen_outline_bounds(const std::vector<Contour>& contours) {
  return bounds_of(contours);
}

std::size_t path_count(const std::vector<Contour>& contours) {
  std::size_t count = 0;
  for (const Contour& contour : contours) {
    if (contour.points.size() >= 3) ++count;
  }
  return count;
}

void append_path(std::ostringstream& output, const std::vector<Vec2>& points,
                 double dx, double dy, const char* indent) {
  if (points.size() < 3) return;
  output << indent << "<path d=\"M " << points[0].x + dx << ' '
         << points[0].y + dy;
  for (std::size_t i = 1; i < points.size(); ++i) {
    output << " L " << points[i].x + dx << ' ' << points[i].y + dy;
  }
  output << " Z\"/>\n";
}

void append_marker(std::ostringstream& output, const char* operation, double width,
                   double height) {
  const double x = 3.5;
  const double y = 3.5;
  output << "  <g id=\"alignment-marker\" data-alignment-marker=\"true\" "
            "data-alignment-marker-operation=\"" << operation << "\" "
            "fill=\"none\" stroke=\"#ff00ff\" stroke-width=\"0.25\">\n"
         << "    <title>ALIGNMENT MARKER | shared by cut and guide</title>\n"
         << "    <circle cx=\"" << x << "\" cy=\"" << y
         << "\" r=\"1.25\"/>\n"
         << "    <path d=\"M " << x - 2.0 << ' ' << y << " L " << x + 2.0
         << ' ' << y << " M " << x << ' ' << y - 2.0 << " L " << x << ' '
         << y + 2.0 << "\"/>\n"
         << "  </g>\n";
  (void)width;
  (void)height;
}

void append_layer_numbers(std::ostringstream& output, const CricutPage& page) {
  if (!page.show_layer_numbers) return;
  output << "  <g id=\"layer-numbers\" data-operation=\"layer-number\">\n";
  for (const CricutTile& tile : page.tiles) {
    const Bounds bounds = bounds_of(tile.contours);
    if (!bounds.set) continue;
    const double center_x = tile.origin_x + (bounds.min_x + bounds.max_x) * 0.5;
    const double center_y = tile.origin_y + (bounds.min_y + bounds.max_y) * 0.5;
    output << "    <text id=\"layer-number-" << std::setw(3)
           << std::setfill('0') << tile.layer_index
           << "\" data-layer-index=\"" << tile.layer_index
           << "\" x=\"" << center_x << "\" y=\"" << center_y
           << "\" font-size=\"" << page.layer_number_font_size_mm
           << "mm\" font-family=\"sans-serif\" fill=\"green\""
              " text-anchor=\"middle\" dominant-baseline=\"middle\">\n"
           << "      <title>LAYER NUMBER | Layer " << std::setw(3)
           << std::setfill('0') << tile.layer_index << " | "
           << (tile.layer_index + 1) << "</title>\n"
           << "      " << (tile.layer_index + 1) << "\n"
           << "    </text>\n";
  }
  output << "  </g>\n";
}

std::string page_header(const CricutPage& page, const char* kind) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::fixed << std::setprecision(9);
  output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\""
         << page.width_mm << "mm\" height=\"" << page.height_mm
         << "mm\" viewBox=\"0 0 " << page.width_mm << ' ' << page.height_mm
         << "\" data-page-index=\"" << page.page_index << "\" data-output=\""
         << kind << "\">\n";
  return output.str();
}

}  // namespace

CricutPageResult build_cricut_pages(
    const std::vector<SliceLayer>& layers, const CricutPageOptions& options) {
  CricutPageResult result;
  if (!std::isfinite(options.gap_mm) || options.gap_mm < -1000000.0 ||
      options.path_limit == 0) {
    result.error = "Invalid Cricut page options";
    return result;
  }

  const double page_width = 304.0;
  const double page_height = options.size == CricutPageSize::NORMAL ? 304.0 : 608.0;
  const double usable_width = page_width - 14.0;
  const double usable_height = page_height - 14.0;
  Bounds model_bounds;
  for (const SliceLayer& layer : layers) {
    const Bounds bounds = bounds_of(layer.contours);
    if (!bounds.set) continue;
    if (!model_bounds.set) {
      model_bounds = bounds;
    } else {
      model_bounds.min_x = std::min(model_bounds.min_x, bounds.min_x);
      model_bounds.min_y = std::min(model_bounds.min_y, bounds.min_y);
      model_bounds.max_x = std::max(model_bounds.max_x, bounds.max_x);
      model_bounds.max_y = std::max(model_bounds.max_y, bounds.max_y);
    }
  }
  if (!model_bounds.set) {
    result.error = "Cricut export contains no non-empty layers";
    return result;
  }

  if (options.packing == CricutPackingStrategy::TIGHT) {
    // Deterministic shelf packing. Layers are considered in input order; each
    // candidate is placed on an existing shelf or starts the next shelf. The
    // globally lowest page/Y/X/orientation candidate wins. Coordinates remain
    // binary doubles until SVG serialization (the writer uses 9 decimals).
    std::vector<TightPageState> pages;
    std::vector<const SliceLayer*> ordered_layers;
    ordered_layers.reserve(layers.size());
    for (const SliceLayer& layer : layers) ordered_layers.push_back(&layer);
    std::stable_sort(ordered_layers.begin(), ordered_layers.end(),
                     [](const SliceLayer* a, const SliceLayer* b) {
                       return a->index < b->index;
                     });
    std::size_t next_page = 1;
    for (std::size_t layer_position = 0; layer_position < ordered_layers.size();
         ++layer_position) {
      const SliceLayer* layer_ptr = ordered_layers[layer_position];
      const SliceLayer& layer = *layer_ptr;
      const Bounds source = bounds_of(layer.contours);
      if (!source.set) continue;
      struct Candidate {
        std::size_t page = 0;
        std::size_t shelf = 0;
        double x = 0.0;
        double y = 0.0;
        double width = 0.0;
        double height = 0.0;
        int orientation = 0;
        std::vector<Contour> contours;
      };
      std::vector<Candidate> candidates;
      for (std::size_t p = 0; p < pages.size(); ++p) {
        for (std::size_t shelf = 0; shelf < pages[p].shelves.size(); ++shelf) {
          for (int orientation : {0, 90, 180, 270}) {
            Bounds normalized;
            auto rotated = rotate_and_normalize(layer.contours, source, orientation,
                                                &normalized);
            const Bounds pen_bounds = pen_outline_bounds(rotated);
            normalized.max_x = std::max(normalized.max_x,
                                        pen_bounds.max_x - pen_bounds.min_x);
            normalized.max_y = std::max(normalized.max_y,
                                        pen_bounds.max_y - pen_bounds.min_y);
            const double x = pages[p].shelves[shelf].next_x;
            const double y = pages[p].shelves[shelf].y;
            if (x + normalized.max_x > usable_width + 1e-9 ||
                y + normalized.max_y > usable_height + 1e-9) continue;
            candidates.push_back({p, shelf, x, y, normalized.max_x,
                                  normalized.max_y, orientation,
                                  std::move(rotated)});
          }
        }
        for (int orientation : {0, 90, 180, 270}) {
          Bounds normalized;
          auto rotated = rotate_and_normalize(layer.contours, source, orientation,
                                              &normalized);
          const Bounds pen_bounds = pen_outline_bounds(rotated);
          normalized.max_x = std::max(normalized.max_x,
                                      pen_bounds.max_x - pen_bounds.min_x);
          normalized.max_y = std::max(normalized.max_y,
                                      pen_bounds.max_y - pen_bounds.min_y);
          const double y = pages[p].shelves.empty()
                               ? 0.0
                               : pages[p].shelves.back().y +
                                     pages[p].shelves.back().height + options.gap_mm;
          if (normalized.max_x <= usable_width + 1e-9 &&
              y + normalized.max_y <= usable_height + 1e-9) {
            candidates.push_back({p, pages[p].shelves.size(), 0.0, y,
                                  normalized.max_x, normalized.max_y, orientation,
                                  std::move(rotated)});
          }
        }
      }
      for (int orientation : {0, 90, 180, 270}) {
        Bounds normalized;
        auto rotated = rotate_and_normalize(layer.contours, source, orientation,
                                            &normalized);
        const Bounds pen_bounds = pen_outline_bounds(rotated);
        normalized.max_x = std::max(normalized.max_x,
                                    pen_bounds.max_x - pen_bounds.min_x);
        normalized.max_y = std::max(normalized.max_y,
                                    pen_bounds.max_y - pen_bounds.min_y);
        if (normalized.max_x <= usable_width + 1e-9 &&
            normalized.max_y <= usable_height + 1e-9) {
          candidates.push_back({pages.size(), 0, 0.0, 0.0, normalized.max_x,
                                normalized.max_y, orientation, std::move(rotated)});
        }
      }
      if (candidates.empty()) {
        result.error = "A layer cannot fit on the selected Cricut page in any allowed orientation";
        return result;
      }
      const auto best = std::min_element(
          candidates.begin(), candidates.end(), [](const Candidate& a,
                                                    const Candidate& b) {
            if (a.page != b.page) return a.page < b.page;
            if (a.y != b.y) return a.y < b.y;
            if (a.x != b.x) return a.x < b.x;
            return a.orientation < b.orientation;
          });
      if (best->page == pages.size()) {
        TightPageState state;
        state.page = {next_page++, page_width, page_height, 0.0, 0.0, {}, 0, true,
                      options.show_layer_numbers, options.layer_number_font_size_mm};
        pages.push_back(std::move(state));
      }
      TightPageState& selected = pages[best->page];
      if (best->shelf == selected.shelves.size()) {
        selected.shelves.push_back({best->y, best->height, best->width + options.gap_mm});
      } else {
        Shelf& shelf = selected.shelves[best->shelf];
        shelf.next_x = best->x + best->width + options.gap_mm;
        shelf.height = std::max(shelf.height, best->height);
      }
      CricutTile tile;
      tile.layer_index = layer.index;
      tile.z = layer.z;
      tile.origin_x = 7.0 + best->x;
      tile.origin_y = 7.0 + best->y;
      tile.width_mm = best->width;
      tile.height_mm = best->height;
      tile.orientation_degrees = best->orientation;
      tile.contours = best->contours;
      selected.page.cut_path_count += path_count(tile.contours);
      if (selected.page.cut_path_count + 1 > options.path_limit) {
        result.error = "Cricut page exceeds the configured SVG path limit";
        return result;
      }
      selected.page.tiles.push_back(std::move(tile));
    }
    for (TightPageState& state : pages) {
      state.page.cut_path_count += 1;
      std::size_t guide_paths = 1;
      for (std::size_t i = 1; i < state.page.tiles.size(); ++i) {
        guide_paths += path_count(state.page.tiles[i].contours);
      }
      if (state.page.cut_path_count > options.path_limit ||
          guide_paths > options.path_limit) {
        result.error = "Cricut page exceeds the configured SVG path limit";
        return result;
      }
      result.pages.push_back(std::move(state.page));
    }
    if (options.gap_mm < 0.0) {
      result.warnings.push_back("Negative Cricut gap causes tile rectangles to overlap");
    }
    return result;
  }

  const double tile_width = model_bounds.max_x - model_bounds.min_x;
  const double tile_height = model_bounds.max_y - model_bounds.min_y;
  if (tile_width > usable_width || tile_height > usable_height) {
    result.error = "A layer exceeds the usable Cricut page area";
    return result;
  }
  const double pitch_x = tile_width + options.gap_mm;
  const double pitch_y = tile_height + options.gap_mm;
  if (pitch_x <= 0.0 || pitch_y <= 0.0) {
    result.error = "Cricut gap makes tile pitch non-positive";
    return result;
  }
  const std::size_t columns =
      static_cast<std::size_t>(std::floor((usable_width + options.gap_mm) / pitch_x));
  const std::size_t rows =
      static_cast<std::size_t>(std::floor((usable_height + options.gap_mm) / pitch_y));
  if (columns == 0 || rows == 0) {
    result.error = "No Cricut tile fits on the selected page";
    return result;
  }
  if (options.gap_mm < 0.0) {
    result.warnings.push_back("Negative Cricut gap causes tile rectangles to overlap");
  }
  const std::size_t per_page = columns * rows;

  std::size_t page_index = 1;
  CricutPage page{page_index, page_width, page_height, tile_width, tile_height, {}, 0,
                  false, options.show_layer_numbers, options.layer_number_font_size_mm};
  for (const SliceLayer& layer : layers) {
    if (layer.contours.empty()) continue;
    const std::size_t page_position = page.tiles.size();
    if (page_position == per_page) {
      page.cut_path_count += 1;  // shared alignment marker
      std::size_t guide_paths = 1;  // shared alignment marker
      for (std::size_t i = 1; i < page.tiles.size(); ++i) {
        guide_paths += path_count(page.tiles[i].contours);
      }
      if (page.cut_path_count > options.path_limit || guide_paths > options.path_limit) {
        result.error = "Cricut page exceeds the configured SVG path limit";
        return result;
      }
      result.pages.push_back(std::move(page));
      page = {++page_index, page_width, page_height, tile_width, tile_height, {}, 0,
              false, options.show_layer_numbers, options.layer_number_font_size_mm};
    }
    const std::size_t position = page.tiles.size();
    CricutTile tile;
    tile.layer_index = layer.index;
    tile.z = layer.z;
    tile.origin_x = 7.0 + static_cast<double>(position % columns) * pitch_x - model_bounds.min_x;
    tile.origin_y = 7.0 + static_cast<double>(position / columns) * pitch_y - model_bounds.min_y;
    tile.width_mm = tile_width;
    tile.height_mm = tile_height;
    tile.contours = layer.contours;
    page.cut_path_count += path_count(tile.contours);
    if (page.cut_path_count > options.path_limit) {
      result.error = "Cricut page exceeds the configured SVG path limit";
      return result;
    }
    page.tiles.push_back(std::move(tile));
  }
  if (!page.tiles.empty()) {
    page.cut_path_count += 1;  // shared alignment marker
    std::size_t guide_paths = 1;
    for (std::size_t i = 1; i < page.tiles.size(); ++i) {
      guide_paths += path_count(page.tiles[i].contours);
    }
    if (page.cut_path_count > options.path_limit || guide_paths > options.path_limit) {
      result.error = "Cricut page exceeds the configured SVG path limit";
      return result;
    }
    result.pages.push_back(std::move(page));
  }
  return result;
}

std::string make_cricut_cut_svg(const CricutPage& page) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << page_header(page, "cut");
  output << std::fixed << std::setprecision(9);
  append_marker(output, "ignore", page.width_mm, page.height_mm);
  for (const CricutTile& tile : page.tiles) {
    output << "  <g id=\"cut-layer-" << std::setw(3) << std::setfill('0')
           << tile.layer_index << "\" fill=\"black\" fill-rule=\"evenodd\""
              " stroke=\"none\" data-layer-index=\""
           << tile.layer_index << "\">\n"
           << "    <title>CUT | Layer " << std::setw(3) << std::setfill('0')
           << tile.layer_index << "</title>\n";
     if (page.tight_packing) {
       output << "    <metadata data-orientation=\"" << tile.orientation_degrees
              << "\" data-bounds=\"0 0 " << tile.width_mm << ' ' << tile.height_mm
              << "\"/>\n";
     }
    for (const Contour& contour : tile.contours) {
      append_path(output, contour.points, tile.origin_x, tile.origin_y, "    ");
    }
    output << "  </g>\n";
  }
  output << "</svg>\n";
  return output.str();
}

std::string make_cricut_guide_svg(const CricutPage& page, double inset_mm,
                                   std::vector<std::string>* warnings) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << page_header(page, "guide");
  output << std::fixed << std::setprecision(9);
  append_marker(output, "draw", page.width_mm, page.height_mm);
  for (std::size_t position = 1; position < page.tiles.size(); ++position) {
    const CricutTile& previous = page.tiles[position - 1];
    const CricutTile& next = page.tiles[position];
    output << "  <g id=\"pen-guide-" << std::setw(3) << std::setfill('0')
           << next.layer_index << "-over-" << std::setw(3) << previous.layer_index
           << "\" fill=\"none\" stroke=\"#1769aa\" stroke-width=\"0.25\""
              " data-guide-layer=\""
           << next.layer_index << "\" data-previous-layer=\"" << previous.layer_index
           << "\">\n"
            << "    <title>PEN GUIDE | Layer " << std::setw(3) << next.layer_index
            << " over Layer " << std::setw(3) << previous.layer_index << " | Inset "
            << inset_mm << "mm</title>\n";
    if (page.tight_packing) {
      output << "    <metadata data-orientation=\"" << next.orientation_degrees
             << "\" data-bounds=\"0 0 " << next.width_mm << ' ' << next.height_mm
             << "\"/>\n";
    }
    std::vector<Contour> guide_contours = next.contours;
#ifdef LAYER_CUT_HAVE_CLIPPER
    if (!std::isfinite(inset_mm) || inset_mm <= 0.0) {
      if (warnings) warnings->push_back("Guide inset must be positive; using outline");
    } else {
      const auto offset = offset_polygons(next.contours, -inset_mm);
      const auto outside = offset.ok()
                               ? difference_polygons(offset.contours, previous.contours)
                               : PolygonOperationResult{};
      if (!offset.ok() || !outside.ok() || !outside.contours.empty() ||
          offset.contours.empty()) {
        if (warnings) {
          warnings->push_back("Guide inset invalid or not contained for layer " +
                              std::to_string(next.layer_index) +
                              "; using outline");
        }
      } else {
        guide_contours = offset.contours;
      }
    }
#else
    if (warnings) warnings->push_back("Guide inset unavailable without Clipper2");
#endif
    for (const Contour& contour : guide_contours) {
      append_path(output, contour.points, previous.origin_x, previous.origin_y, "    ");
    }
    output << "  </g>\n";
  }
  output << "</svg>\n";
  return output.str();
}

std::string make_cricut_combined_svg(const CricutPage& page, double inset_mm,
                                     std::vector<std::string>* warnings) {
  std::ostringstream output;
  output.imbue(std::locale::classic());
  (void)inset_mm;
  output << page_header(page, "combined");
  output << std::fixed << std::setprecision(9);
  output << "  <g id=\"cut-layers\" data-operation=\"cut\">\n";
  for (const CricutTile& tile : page.tiles) {
    output << "    <g id=\"cut-layer-" << std::setw(3) << std::setfill('0')
           << tile.layer_index << "\" fill=\"black\" fill-rule=\"evenodd\""
               " stroke=\"none\" data-layer-index=\""
            << tile.layer_index << "\">\n";
    if (page.tight_packing) {
      output << "      <metadata data-orientation=\"" << tile.orientation_degrees
             << "\" data-bounds=\"0 0 " << tile.width_mm << ' ' << tile.height_mm
             << "\"/>\n";
    }
    for (const Contour& contour : tile.contours) {
      append_path(output, contour.points, tile.origin_x, tile.origin_y, "      ");
    }
    output << "    </g>\n";
  }
  output << "  </g>\n  <g id=\"pen-layers\" data-operation=\"draw\">\n";
  for (std::size_t position = 1; position < page.tiles.size(); ++position) {
    const CricutTile& previous = page.tiles[position - 1];
    const CricutTile& next = page.tiles[position];
    std::vector<Contour> guide_contours = next.contours;
#ifdef LAYER_CUT_HAVE_CLIPPER
    if (!std::isfinite(inset_mm) || inset_mm <= 0.0) {
      if (warnings) warnings->push_back("Guide inset must be positive; using outline");
    } else {
      const auto offset = offset_polygons(next.contours, -inset_mm);
      const auto outside = offset.ok()
                               ? difference_polygons(offset.contours, previous.contours)
                               : PolygonOperationResult{};
      if (!offset.ok() || !outside.ok() || !outside.contours.empty() ||
          offset.contours.empty()) {
        if (warnings) {
          warnings->push_back("Guide inset invalid or not contained for layer " +
                              std::to_string(next.layer_index) +
                              "; using outline");
        }
      } else {
        guide_contours = offset.contours;
      }
    }
#else
    if (warnings) warnings->push_back("Guide inset unavailable without Clipper2");
#endif
    output << "    <g id=\"pen-guide-" << std::setw(3) << std::setfill('0')
           << next.layer_index << "-over-" << std::setw(3) << previous.layer_index
           << "\" fill=\"none\" stroke=\"#1769aa\" stroke-width=\"0.25\""
              " data-guide-layer=\""
            << next.layer_index << "\" data-previous-layer=\"" << previous.layer_index
            << "\">\n";
    if (page.tight_packing) {
      output << "      <metadata data-orientation=\"" << next.orientation_degrees
             << "\" data-bounds=\"0 0 " << next.width_mm << ' ' << next.height_mm
             << "\"/>\n";
    }
    for (const Contour& contour : guide_contours) {
      append_path(output, contour.points, previous.origin_x, previous.origin_y, "      ");
    }
    output << "    </g>\n";
  }
  output << "  </g>\n";
  append_layer_numbers(output, page);
  output << "</svg>\n";
  return output.str();
}

}  // namespace layer_cut
