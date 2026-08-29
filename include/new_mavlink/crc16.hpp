#pragma once
#include <cstddef>
#include <cstdint>
namespace new_mavlink {
std::uint16_t crc16_ccitt(const std::uint8_t* bytes, std::size_t length) noexcept;
}
