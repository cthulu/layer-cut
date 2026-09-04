#include "png_writer.h"

#include <cstdint>
#include <string>

int main() {
  layer_cut::Contour square;
  square.points = {{0, 0}, {25.4, 0}, {25.4, 25.4}, {0, 25.4}};
  const auto result = layer_cut::make_png({square}, {{0, 0}, {25.4, 25.4}, 300});
  if (!result.ok() || result.width != 300 || result.height != 300 || result.bytes.size() < 33) return 1;
  const std::string signature("\x89PNG\r\n\x1a\n", 8);
  if (std::string(reinterpret_cast<const char*>(result.bytes.data()), 8) != signature) return 2;
  bool has_phys = false;
  for (std::size_t i = 8; i + 4 < result.bytes.size(); ++i) {
    if (result.bytes[i] == 'p' && result.bytes[i + 1] == 'H' &&
        result.bytes[i + 2] == 'Y' && result.bytes[i + 3] == 's') has_phys = true;
  }
  return has_phys ? 0 : 3;
}
