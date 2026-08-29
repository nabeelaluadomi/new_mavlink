#include "new_mavlink/messages.hpp"

#include <array>
#include <cassert>

int main() {
  using namespace new_mavlink;
  std::array<std::uint8_t, kCommandBytes> bytes{};
  CommandRequest in{}; in.request_id = 7U; in.command_id = 400U; in.deadline_ms = 1000U; in.arg_count = 2U; in.args[0] = 1.5F; in.args[1] = -2.0F;
  std::size_t length = 0U; assert(encode_command(in, bytes.data(), bytes.size(), length)); assert(length == kCommandBytes);
  CommandRequest out{}; assert(decode_command(bytes.data(), length, out)); assert(out.request_id == in.request_id && out.command_id == in.command_id && out.args[1] == in.args[1]);
  bytes[12] = static_cast<std::uint8_t>(kMaxArgs + 1U); assert(!decode_command(bytes.data(), length, out));
  std::array<std::uint8_t, kPositionBytes> position_bytes{}; Position position{}; position.meta = StateMeta{9U, 1U, 0U, 1U, 1U, 0U}; position.latitude_e7 = 473977400; position.longitude_e7 = 85455900; position.altitude_mm = 500000; assert(encode_position(position, position_bytes.data(), position_bytes.size())); Position decoded_position{}; assert(decode_position(position_bytes.data(), position_bytes.size(), decoded_position)); assert(decoded_position.latitude_e7 == position.latitude_e7 && decoded_position.altitude_mm == position.altitude_mm);
  std::array<std::uint8_t, kAttitudeBytes> attitude_bytes{}; Attitude attitude{}; attitude.meta = position.meta; attitude.roll_urad = 100000; attitude.pitch_urad = -200000; attitude.yaw_urad = 300000; assert(encode_attitude(attitude, attitude_bytes.data(), attitude_bytes.size())); Attitude decoded_attitude{}; assert(decode_attitude(attitude_bytes.data(), attitude_bytes.size(), decoded_attitude)); assert(decoded_attitude.pitch_urad == -200000);
  std::array<std::uint8_t, kBatteryBytes> battery_bytes{}; Battery battery{}; battery.meta = position.meta; battery.voltage_mv = 16000U; battery.current_ma = 1200U; battery.remaining_permille = 930U; assert(encode_battery(battery, battery_bytes.data(), battery_bytes.size())); Battery decoded_battery{}; assert(decode_battery(battery_bytes.data(), battery_bytes.size(), decoded_battery)); assert(decoded_battery.remaining_permille == 930U);
  std::array<std::uint8_t, kHeartbeatBytes> heartbeat_bytes{}; Heartbeat heartbeat{1234U, 7U, 2U}; assert(encode_heartbeat(heartbeat, heartbeat_bytes.data(), heartbeat_bytes.size())); Heartbeat decoded_heartbeat{}; assert(decode_heartbeat(heartbeat_bytes.data(), heartbeat_bytes.size(), decoded_heartbeat)); assert(decoded_heartbeat.uptime_ms == 1234U);
  battery.remaining_permille = 1001U; assert(!encode_battery(battery, battery_bytes.data(), battery_bytes.size()));
  return 0;
}
