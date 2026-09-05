#include "png_writer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <cstdint>

#ifdef LAYER_CUT_HAVE_STB
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#endif

namespace layer_cut {
namespace {

std::uint32_t crc32(const std::uint8_t* data, std::size_t size) {
  std::uint32_t crc = 0xffffffffu;
  for (std::size_t i = 0; i < size; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xedb88320u & -(crc & 1u));
  }
  return ~crc;
}

void append_u32(std::vector<std::uint8_t>& output, std::uint32_t value) {
  output.push_back(static_cast<std::uint8_t>(value >> 24));
  output.push_back(static_cast<std::uint8_t>(value >> 16));
  output.push_back(static_cast<std::uint8_t>(value >> 8));
  output.push_back(static_cast<std::uint8_t>(value));
}

bool add_phys_chunk(std::vector<std::uint8_t>& png, int dpi) {
  const std::uint32_t pixels_per_metre = static_cast<std::uint32_t>(
      std::llround(static_cast<double>(dpi) / 0.0254));
  std::vector<std::uint8_t> chunk{'p', 'H', 'Y', 's'};
  append_u32(chunk, pixels_per_metre);
  append_u32(chunk, pixels_per_metre);
  chunk.push_back(1);  // metres
  std::vector<std::uint8_t> encoded;
  append_u32(encoded, static_cast<std::uint32_t>(chunk.size() - 4));
  encoded.insert(encoded.end(), chunk.begin(), chunk.end());
  append_u32(encoded, crc32(chunk.data(), chunk.size()));
  std::size_t insert_at = 8;
  bool found_idat = false;
  while (insert_at + 8 <= png.size()) {
    const std::uint32_t length = (static_cast<std::uint32_t>(png[insert_at]) << 24) |
        (static_cast<std::uint32_t>(png[insert_at + 1]) << 16) |
        (static_cast<std::uint32_t>(png[insert_at + 2]) << 8) | png[insert_at + 3];
    if (insert_at + 12 + length > png.size()) break;
    if (png[insert_at + 4] == 'I' && png[insert_at + 5] == 'D' &&
        png[insert_at + 6] == 'A' && png[insert_at + 7] == 'T') {
      found_idat = true;
      break;
    }
    insert_at += 12 + length;
  }
  if (!found_idat) return false;
  png.insert(png.begin() + static_cast<std::ptrdiff_t>(insert_at), encoded.begin(), encoded.end());
  return true;
}

bool inside(const std::vector<Contour>& contours, double x, double y) {
  bool result = false;
  for (const Contour& contour : contours) {
    const auto& points = contour.points;
    if (points.size() < 3) continue;
    for (std::size_t i = 0, j = points.size() - 1; i < points.size(); j = i++) {
      const Vec2& a = points[i];
      const Vec2& b = points[j];
      if ((a.y > y) != (b.y > y) &&
          x < (b.x - a.x) * (y - a.y) / (b.y - a.y) + a.x) {
        result = !result;
      }
    }
  }
  return result;
}

}  // namespace

PngResult make_png(const std::vector<Contour>& contours, const PngOptions& options) {
  PngResult result;
  const double width_mm = options.max.x - options.min.x;
  const double height_mm = options.max.y - options.min.y;
  if (!std::isfinite(width_mm) || !std::isfinite(height_mm) || width_mm <= 0.0 ||
      height_mm <= 0.0 || options.dpi <= 0) {
    result.error = "PNG bounds must be positive and DPI must be positive";
    return result;
  }
  const double scale = static_cast<double>(options.dpi) / 25.4;
  const double width_px = std::ceil(width_mm * scale);
  const double height_px = std::ceil(height_mm * scale);
  constexpr std::uint64_t max_pixels = 100000000ULL;
  constexpr std::uint64_t max_bytes = 400000000ULL;
  if (width_px > static_cast<double>(std::numeric_limits<std::uint64_t>::max()) ||
      height_px > static_cast<double>(std::numeric_limits<std::uint64_t>::max())) {
    result.error = "PNG dimensions exceed the safety limit";
    return result;
  }
  const std::uint64_t width_count = static_cast<std::uint64_t>(width_px);
  const std::uint64_t height_count = static_cast<std::uint64_t>(height_px);
  if (width_count == 0 || height_count == 0 || width_count > max_pixels ||
      height_count > max_pixels || width_count > max_pixels / height_count) {
    result.error = "PNG dimensions exceed the safety limit";
    return result;
  }
  const std::uint64_t pixel_count = width_count * height_count;
  if (pixel_count > max_pixels || pixel_count > max_bytes / 4 ||
      pixel_count > std::numeric_limits<std::size_t>::max() / 4 ||
      pixel_count > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
    result.error = "PNG dimensions exceed the safety limit";
    return result;
  }
  result.width = static_cast<int>(width_count);
  result.height = static_cast<int>(height_count);
  std::vector<unsigned char> pixels(static_cast<std::size_t>(pixel_count) * 4,
                                    options.black_on_white ? 255 : 0);
  constexpr int samples_per_axis = 4;
  constexpr int sample_count = samples_per_axis * samples_per_axis;
  for (int py = 0; py < result.height; ++py) {
    for (int px = 0; px < result.width; ++px) {
      int covered = 0;
      if (options.antialias) {
        for (int sy = 0; sy < samples_per_axis; ++sy) {
          for (int sx = 0; sx < samples_per_axis; ++sx) {
            const double x = options.min.x + (px + (sx + 0.5) / samples_per_axis) / scale;
            const double y = options.min.y + (py + (sy + 0.5) / samples_per_axis) / scale;
            if (inside(contours, x, y)) ++covered;
          }
        }
      } else {
        const double x = options.min.x + (px + 0.5) / scale;
        const double y = options.min.y + (py + 0.5) / scale;
        covered = inside(contours, x, y) ? sample_count : 0;
      }
      const double coverage = static_cast<double>(covered) / sample_count;
      const unsigned char background = options.black_on_white ? 255 : 0;
      const unsigned char foreground = options.black_on_white ? 0 : 255;
      const unsigned char value = static_cast<unsigned char>(std::lround(
          background + (foreground - background) * coverage));
      const std::size_t offset = (static_cast<std::size_t>(py) * result.width + px) * 4;
      pixels[offset] = pixels[offset + 1] = pixels[offset + 2] = value;
      pixels[offset + 3] = 255;
    }
  }
#ifdef LAYER_CUT_HAVE_STB
  auto callback = [](void* context, void* data, int size) {
    auto* output = static_cast<std::vector<std::uint8_t>*>(context);
    const auto* begin = static_cast<const std::uint8_t*>(data);
    output->insert(output->end(), begin, begin + size);
  };
  stbi_write_png_to_func(callback, &result.bytes, result.width, result.height, 4,
                         pixels.data(), result.width * 4);
  if (result.bytes.empty()) result.error = "PNG encoding failed";
  else if (!add_phys_chunk(result.bytes, options.dpi)) result.error = "Failed to add physical DPI metadata to PNG";
#else
  result.error = "PNG support was not compiled (enable dependency fetching)";
#endif
  return result;
}

}  // namespace layer_cut
