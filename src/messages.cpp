#include "new_mavlink/messages.hpp"

#include <cmath>
#include <cstring>

namespace new_mavlink {
namespace {

void put16(std::uint8_t* p, std::uint16_t v) noexcept { p[0] = static_cast<std::uint8_t>(v); p[1] = static_cast<std::uint8_t>(v >> 8U); }
void put32(std::uint8_t* p, std::uint32_t v) noexcept { for (unsigned i = 0U; i < 4U; ++i) p[i] = static_cast<std::uint8_t>(v >> (8U * i)); }
void put64(std::uint8_t* p, std::uint64_t v) noexcept { for (unsigned i = 0U; i < 8U; ++i) p[i] = static_cast<std::uint8_t>(v >> (8U * i)); }
std::uint16_t get16(const std::uint8_t* p) noexcept { return static_cast<std::uint16_t>(p[0]) | static_cast<std::uint16_t>(p[1] << 8U); }
std::uint32_t get32(const std::uint8_t* p) noexcept { std::uint32_t v = 0U; for (unsigned i = 0U; i < 4U; ++i) v |= static_cast<std::uint32_t>(p[i]) << (8U * i); return v; }
std::uint64_t get64(const std::uint8_t* p) noexcept { std::uint64_t v = 0U; for (unsigned i = 0U; i < 8U; ++i) v |= static_cast<std::uint64_t>(p[i]) << (8U * i); return v; }
void put_float(std::uint8_t* p, float value) noexcept { static_assert(sizeof(float) == 4U, "IEEE float required"); std::uint32_t bits = 0U; std::memcpy(&bits, &value, sizeof(bits)); put32(p, bits); }
float get_float(const std::uint8_t* p) noexcept { std::uint32_t bits = get32(p); float value = 0.0F; std::memcpy(&value, &bits, sizeof(value)); return value; }

bool valid_command(const CommandRequest& value) noexcept {
  if (value.request_id == 0U || value.command_id == 0U || value.deadline_ms == 0U ||
      value.arg_count > kMaxArgs) return false;
  for (std::size_t i = 0U; i < value.arg_count; ++i) if (!std::isfinite(value.args[i])) return false;
  return true;
}

} // namespace

bool encode_command(const CommandRequest& value, std::uint8_t* out, std::size_t capacity, std::size_t& length) noexcept {
  length = 0U;
  if (out == nullptr || capacity < kCommandBytes || !valid_command(value)) return false;
  put64(out, value.request_id); put16(out + 8U, value.command_id); put16(out + 10U, value.deadline_ms); out[12] = value.arg_count; out[13] = 0U; out[14] = 0U; out[15] = 0U;
  for (std::size_t i = 0U; i < kMaxArgs; ++i) put_float(out + 16U + 4U * i, value.args[i]);
  length = kCommandBytes; return true;
}

bool decode_command(const std::uint8_t* bytes, std::size_t length, CommandRequest& value) noexcept {
  value = CommandRequest{};
  if (bytes == nullptr || length != kCommandBytes || bytes[13] != 0U || bytes[14] != 0U || bytes[15] != 0U) return false;
  value.request_id = get64(bytes); value.command_id = get16(bytes + 8U); value.deadline_ms = get16(bytes + 10U); value.arg_count = bytes[12];
  for (std::size_t i = 0U; i < kMaxArgs; ++i) value.args[i] = get_float(bytes + 16U + 4U * i);
  return valid_command(value);
}

bool encode_ack(const CommandAck& value, std::uint8_t* out, std::size_t capacity, std::size_t& length) noexcept {
  length = 0U; if (out == nullptr || capacity < kAckBytes || value.request_id == 0U) return false;
  put64(out, value.request_id); out[8] = value.status; out[9] = value.detail; put16(out + 10U, value.reserved); put32(out + 12U, value.result_code); length = kAckBytes; return true;
}

bool decode_ack(const std::uint8_t* bytes, std::size_t length, CommandAck& value) noexcept {
  value = CommandAck{}; if (bytes == nullptr || length != kAckBytes) return false;
  value.request_id = get64(bytes); value.status = bytes[8]; value.detail = bytes[9]; value.reserved = get16(bytes + 10U); value.result_code = get32(bytes + 12U); return value.request_id != 0U;
}

bool encode_subscription(const Subscription& value, std::uint8_t* out, std::size_t capacity, std::size_t& length) noexcept {
  length = 0U; if (out == nullptr || capacity < kSubscribeBytes || !known_type(value.type) || value.source_id == 0U) return false;
  out[0] = static_cast<std::uint8_t>(value.type); out[1] = static_cast<std::uint8_t>(value.qos); put16(out + 2U, value.source_id); put16(out + 4U, value.interval_ms); put32(out + 6U, value.flags); length = kSubscribeBytes; return true;
}

bool decode_subscription(const std::uint8_t* bytes, std::size_t length, Subscription& value) noexcept {
  value = Subscription{}; if (bytes == nullptr || length != kSubscribeBytes || !known_type(static_cast<MessageType>(bytes[0])) || bytes[1] > static_cast<std::uint8_t>(Qos::Explicit)) return false;
  value.type = static_cast<MessageType>(bytes[0]); value.qos = static_cast<Qos>(bytes[1]); value.source_id = get16(bytes + 2U); value.interval_ms = get16(bytes + 4U); value.flags = get32(bytes + 6U); return value.source_id != 0U;
}

bool encode_unsubscribe(const Subscription& value, std::uint8_t* out, std::size_t capacity, std::size_t& length) noexcept {
  return encode_subscription(value, out, capacity, length);
}

bool decode_unsubscribe(const std::uint8_t* bytes, std::size_t length, Subscription& value) noexcept {
  return decode_subscription(bytes, length, value);
}

bool encode_resync(const ResyncRequest& value, std::uint8_t* out, std::size_t capacity, std::size_t& length) noexcept {
  length = 0U; if (out == nullptr || capacity < kResyncBytes || !state_type(value.type) || value.source_id == 0U || value.resource_id == 0U) return false;
  out[0] = static_cast<std::uint8_t>(value.type); out[1] = 0U; put16(out + 2U, value.source_id); put32(out + 4U, value.resource_id); length = kResyncBytes; return true;
}

bool decode_resync(const std::uint8_t* bytes, std::size_t length, ResyncRequest& value) noexcept {
  value = ResyncRequest{}; if (bytes == nullptr || length != kResyncBytes || bytes[1] != 0U || !state_type(static_cast<MessageType>(bytes[0]))) return false;
  value.type = static_cast<MessageType>(bytes[0]); value.source_id = get16(bytes + 2U); value.resource_id = get32(bytes + 4U); return value.source_id != 0U && value.resource_id != 0U;
}

bool encode_state_meta(const StateMeta& value, std::uint8_t* out, std::size_t capacity) noexcept {
  if (out == nullptr || capacity < 16U || value.resource_id == 0U || value.generation == 0U || value.source_id == 0U) return false;
  put32(out, value.resource_id); put32(out + 4U, value.generation); put32(out + 8U, value.base_generation); put16(out + 12U, value.source_id); out[14] = value.snapshot; out[15] = value.reserved; return true;
}

bool decode_state_meta(const std::uint8_t* bytes, std::size_t length, StateMeta& value) noexcept {
  value = StateMeta{}; if (bytes == nullptr || length != 16U) return false;
  value.resource_id = get32(bytes); value.generation = get32(bytes + 4U); value.base_generation = get32(bytes + 8U); value.source_id = get16(bytes + 12U); value.snapshot = bytes[14]; value.reserved = bytes[15]; return value.resource_id != 0U && value.generation != 0U && value.source_id != 0U && value.snapshot <= 1U && value.reserved == 0U;
}

bool encode_position(const Position& value, std::uint8_t* out, std::size_t capacity) noexcept {
  if (out == nullptr || capacity < kPositionBytes || value.meta.snapshot > 1U || value.meta.reserved != 0U || value.latitude_e7 < -900000000 || value.latitude_e7 > 900000000 || value.longitude_e7 < -1800000000 || value.longitude_e7 > 1800000000) return false;
  if (!encode_state_meta(value.meta, out, capacity)) return false; put32(out + 16U, static_cast<std::uint32_t>(value.latitude_e7)); put32(out + 20U, static_cast<std::uint32_t>(value.longitude_e7)); put32(out + 24U, static_cast<std::uint32_t>(value.altitude_mm)); return true;
}

bool decode_position(const std::uint8_t* bytes, std::size_t length, Position& value) noexcept {
  value = Position{}; if (bytes == nullptr || length != kPositionBytes || !decode_state_meta(bytes, 16U, value.meta)) return false; value.latitude_e7 = static_cast<std::int32_t>(get32(bytes + 16U)); value.longitude_e7 = static_cast<std::int32_t>(get32(bytes + 20U)); value.altitude_mm = static_cast<std::int32_t>(get32(bytes + 24U)); return value.latitude_e7 >= -900000000 && value.latitude_e7 <= 900000000 && value.longitude_e7 >= -1800000000 && value.longitude_e7 <= 1800000000;
}

bool encode_attitude(const Attitude& value, std::uint8_t* out, std::size_t capacity) noexcept {
  if (out == nullptr || capacity < kAttitudeBytes || value.meta.snapshot > 1U || value.meta.reserved != 0U || !encode_state_meta(value.meta, out, capacity)) return false;
  put32(out + 16U, static_cast<std::uint32_t>(value.roll_urad)); put32(out + 20U, static_cast<std::uint32_t>(value.pitch_urad)); put32(out + 24U, static_cast<std::uint32_t>(value.yaw_urad)); return true;
}

bool decode_attitude(const std::uint8_t* bytes, std::size_t length, Attitude& value) noexcept {
  value = Attitude{}; if (bytes == nullptr || length != kAttitudeBytes || !decode_state_meta(bytes, 16U, value.meta)) return false; value.roll_urad = static_cast<std::int32_t>(get32(bytes + 16U)); value.pitch_urad = static_cast<std::int32_t>(get32(bytes + 20U)); value.yaw_urad = static_cast<std::int32_t>(get32(bytes + 24U)); return true;
}

bool encode_battery(const Battery& value, std::uint8_t* out, std::size_t capacity) noexcept {
  if (out == nullptr || capacity < kBatteryBytes || value.meta.snapshot > 1U || value.meta.reserved != 0U || value.remaining_permille > 1000U || value.reserved != 0U || !encode_state_meta(value.meta, out, capacity)) return false;
  put16(out + 16U, value.voltage_mv); put16(out + 18U, value.current_ma); put16(out + 20U, value.remaining_permille); put16(out + 22U, value.reserved); return true;
}

bool decode_battery(const std::uint8_t* bytes, std::size_t length, Battery& value) noexcept {
  value = Battery{}; if (bytes == nullptr || length != kBatteryBytes || !decode_state_meta(bytes, 16U, value.meta)) return false; value.voltage_mv = get16(bytes + 16U); value.current_ma = get16(bytes + 18U); value.remaining_permille = get16(bytes + 20U); value.reserved = get16(bytes + 22U); return value.remaining_permille <= 1000U && value.reserved == 0U;
}

bool encode_heartbeat(const Heartbeat& value, std::uint8_t* out, std::size_t capacity) noexcept {
  if (out == nullptr || capacity < kHeartbeatBytes) return false; put64(out, value.uptime_ms); put32(out + 8U, value.capabilities); put32(out + 12U, value.state); return true;
}

bool decode_heartbeat(const std::uint8_t* bytes, std::size_t length, Heartbeat& value) noexcept {
  value = Heartbeat{}; if (bytes == nullptr || length != kHeartbeatBytes) return false; value.uptime_ms = get64(bytes); value.capabilities = get32(bytes + 8U); value.state = get32(bytes + 12U); return true;
}

} // namespace new_mavlink
