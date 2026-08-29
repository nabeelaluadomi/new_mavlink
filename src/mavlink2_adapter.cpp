#include "new_mavlink/mavlink2_adapter.hpp"

extern "C" {
#include "mavlink.h"
}

#include <cmath>
#include <cstdint>

namespace new_mavlink::mavlink2_adapter {

bool command_allowed(std::uint16_t command_id) noexcept {
  return command_id == 400U || command_id == 22U || command_id == 21U;
}

bool encode_command_long(const CommandRequest& request,
                         std::uint8_t source_system,
                         std::uint8_t source_component,
                         std::uint8_t target_system,
                         std::uint8_t target_component,
                         MavlinkFrame& frame) noexcept {
  frame.length = 0U;
  if (!command_allowed(request.command_id) || request.request_id == 0U ||
      request.deadline_ms == 0U || request.arg_count > kMaxArgs ||
      source_system == 0U || source_component == 0U || target_system == 0U ||
      target_component == 0U) return false;
  for (std::size_t i = 0U; i < request.arg_count; ++i) if (!std::isfinite(request.args[i])) return false;
  mavlink_message_t message{};
  mavlink_msg_command_long_pack(source_system, source_component, &message,
                                target_system, target_component,
                                request.command_id, 0U,
                                request.args[0], request.args[1], request.args[2],
                                request.args[3], request.args[4], request.args[5],
                                request.args[6]);
  const std::uint16_t length = mavlink_msg_to_send_buffer(frame.bytes.data(), &message);
  if (length == 0U || length > frame.bytes.size()) return false;
  frame.length = length; return true;
}

bool decode_command_ack(const std::uint8_t* bytes, std::size_t length,
                        CommandAck& ack) noexcept {
  ack = CommandAck{};
  if (bytes == nullptr || length == 0U || length > 280U) return false;
  mavlink_message_t message{}; mavlink_status_t status{}; bool found = false;
  for (std::size_t i = 0U; i < length; ++i) {
    if (mavlink_parse_char(MAVLINK_COMM_0, bytes[i], &message, &status) && message.msgid == MAVLINK_MSG_ID_COMMAND_ACK) { found = true; break; }
  }
  if (!found) return false;
  mavlink_command_ack_t value{}; mavlink_msg_command_ack_decode(&message, &value);
  if (!command_allowed(value.command)) return false;
  ack.request_id = 1U; ack.status = static_cast<std::uint8_t>(value.result); ack.detail = value.progress; ack.result_code = value.command; return true;
}

namespace {

bool parse_message(const std::uint8_t* bytes, std::size_t length, std::uint32_t msgid, mavlink_message_t& result) noexcept {
  if (bytes == nullptr || length == 0U || length > 280U) return false;
  mavlink_status_t status{};
  for (std::size_t i = 0U; i < length; ++i) {
    if (mavlink_parse_char(MAVLINK_COMM_0, bytes[i], &result, &status) && result.msgid == msgid) return true;
  }
  return false;
}

} // namespace

bool decode_battery_status(const std::uint8_t* bytes, std::size_t length, BatteryTelemetry& value) noexcept {
  value = BatteryTelemetry{}; mavlink_message_t message{}; if (!parse_message(bytes, length, MAVLINK_MSG_ID_BATTERY_STATUS, message)) return false;
  mavlink_battery_status_t raw{}; mavlink_msg_battery_status_decode(&message, &raw); if (message.sysid == 0U || raw.voltages[0] == UINT16_MAX) return false;
  value.source_system = message.sysid; value.voltage_mv = raw.voltages[0]; value.remaining_permille = raw.battery_remaining < 0 ? 0U : raw.battery_remaining >= 100 ? 1000U : static_cast<std::uint16_t>(raw.battery_remaining * 10U); return true;
}

bool decode_global_position(const std::uint8_t* bytes, std::size_t length, PositionTelemetry& value) noexcept {
  value = PositionTelemetry{}; mavlink_message_t message{}; if (!parse_message(bytes, length, MAVLINK_MSG_ID_GLOBAL_POSITION_INT, message)) return false;
  mavlink_global_position_int_t raw{}; mavlink_msg_global_position_int_decode(&message, &raw); if (message.sysid == 0U) return false;
  value.source_system = message.sysid; value.latitude_e7 = raw.lat; value.longitude_e7 = raw.lon; value.altitude_mm = raw.alt; return true;
}

bool decode_attitude(const std::uint8_t* bytes, std::size_t length, AttitudeTelemetry& value) noexcept {
  value = AttitudeTelemetry{}; mavlink_message_t message{}; if (!parse_message(bytes, length, MAVLINK_MSG_ID_ATTITUDE, message)) return false;
  mavlink_attitude_t raw{}; mavlink_msg_attitude_decode(&message, &raw); if (message.sysid == 0U || !std::isfinite(raw.roll) || !std::isfinite(raw.pitch) || !std::isfinite(raw.yaw)) return false;
  constexpr float kRadToUrad = 1000000.0F; value.source_system = message.sysid; value.roll_urad = static_cast<std::int32_t>(raw.roll * kRadToUrad); value.pitch_urad = static_cast<std::int32_t>(raw.pitch * kRadToUrad); value.yaw_urad = static_cast<std::int32_t>(raw.yaw * kRadToUrad); return true;
}

} // namespace new_mavlink::mavlink2_adapter
