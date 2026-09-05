#include "png_writer.h"

#include <cstdint>
#include <string>

namespace {

std::uint32_t read_u32(const std::vector<std::uint8_t>& bytes,
                       std::size_t offset) {
  return (static_cast<std::uint32_t>(bytes[offset]) << 24) |
         (static_cast<std::uint32_t>(bytes[offset + 1]) << 16) |
         (static_cast<std::uint32_t>(bytes[offset + 2]) << 8) |
         bytes[offset + 3];
}

bool has_phys_dpi(const std::vector<std::uint8_t>& bytes, int dpi) {
  const std::uint32_t expected =
      static_cast<std::uint32_t>(dpi / 0.0254 + 0.5);
  std::size_t offset = 8;
  while (offset + 12 <= bytes.size()) {
    const std::uint32_t length = read_u32(bytes, offset);
    if (offset + 12ULL + length > bytes.size()) return false;
    if (bytes[offset + 4] == 'p' && bytes[offset + 5] == 'H' &&
        bytes[offset + 6] == 'Y' && bytes[offset + 7] == 's') {
      return length == 9 && read_u32(bytes, offset + 8) == expected &&
             read_u32(bytes, offset + 12) == expected &&
             bytes[offset + 16] == 1;
    }
    offset += 12 + length;
  }
  return false;
}

}  // namespace

int main() {
  layer_cut::Contour square;
  square.points = {{0, 0}, {25.4, 0}, {25.4, 25.4}, {0, 25.4}};
  const auto result = layer_cut::make_png({square}, {{0, 0}, {25.4, 25.4}, 300});
  if (!result.ok() || result.width != 300 || result.height != 300 || result.bytes.size() < 33) return 1;
  const std::string signature("\x89PNG\r\n\x1a\n", 8);
  if (std::string(reinterpret_cast<const char*>(result.bytes.data()), 8) != signature) return 2;
  if (!has_phys_dpi(result.bytes, 300)) return 3;
  if (layer_cut::make_png({square}, {{0, 0}, {1000000, 1000000}, 1200}).ok()) return 4;
  layer_cut::Contour diagonal;
  diagonal.points = {{0, 0}, {25.4, 0}, {0, 25.4}};
  layer_cut::PngOptions antialiased_options{{0, 0}, {25.4, 25.4}, 10, true, true};
  layer_cut::PngOptions aliased_options = antialiased_options;
  aliased_options.antialias = false;
  const auto antialiased = layer_cut::make_png({diagonal}, antialiased_options);
  const auto aliased = layer_cut::make_png({diagonal}, aliased_options);
  if (!antialiased.ok() || !aliased.ok() || antialiased.bytes == aliased.bytes) return 5;
  return 0;
}
