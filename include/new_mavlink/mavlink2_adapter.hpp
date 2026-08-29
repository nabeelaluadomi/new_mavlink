#pragma once

#include "new_mavlink/messages.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace new_mavlink::mavlink2_adapter {

constexpr std::uint8_t kPx4SystemId = 1U;
constexpr std::uint8_t kPx4ComponentId = 1U;

struct MavlinkFrame {
  std::array<std::uint8_t, 280U> bytes{};
  std::size_t length{0U};
};

bool command_allowed(std::uint16_t command_id) noexcept;
bool encode_command_long(const CommandRequest& request,
                         std::uint8_t source_system,
                         std::uint8_t source_component,
                         std::uint8_t target_system,
                         std::uint8_t target_component,
                         MavlinkFrame& frame) noexcept;
bool decode_command_ack(const std::uint8_t* bytes, std::size_t length,
                        CommandAck& ack) noexcept;

struct BatteryTelemetry {
  std::uint8_t source_system{0U};
  std::uint16_t voltage_mv{0U};
  std::uint16_t remaining_permille{0U};
};
struct PositionTelemetry {
  std::uint8_t source_system{0U};
  std::int32_t latitude_e7{0};
  std::int32_t longitude_e7{0};
  std::int32_t altitude_mm{0};
};
struct AttitudeTelemetry {
  std::uint8_t source_system{0U};
  std::int32_t roll_urad{0};
  std::int32_t pitch_urad{0};
  std::int32_t yaw_urad{0};
};

bool decode_battery_status(const std::uint8_t* bytes, std::size_t length,
                           BatteryTelemetry& value) noexcept;
bool decode_global_position(const std::uint8_t* bytes, std::size_t length,
                            PositionTelemetry& value) noexcept;
bool decode_attitude(const std::uint8_t* bytes, std::size_t length,
                     AttitudeTelemetry& value) noexcept;

} // namespace new_mavlink::mavlink2_adapter
