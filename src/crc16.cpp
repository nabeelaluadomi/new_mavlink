#include "new_mavlink/crc16.hpp"

namespace new_mavlink {

std::uint16_t crc16_ccitt(const std::uint8_t* bytes, std::size_t length) noexcept {
  if (bytes == nullptr && length != 0U) {
    return 0U;
  }
  std::uint16_t crc = 0xFFFFU;
  for (std::size_t i = 0U; i < length; ++i) {
    crc ^= static_cast<std::uint16_t>(bytes[i]) << 8U;
    for (unsigned bit = 0U; bit < 8U; ++bit) {
      crc = (crc & 0x8000U) != 0U
                ? static_cast<std::uint16_t>((crc << 1U) ^ 0x1021U)
                : static_cast<std::uint16_t>(crc << 1U);
    }
  }
  return crc;
}

} // namespace new_mavlink
