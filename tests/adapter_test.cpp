#include "new_mavlink/mavlink2_adapter.hpp"
extern "C" {
#include "mavlink.h"
}

#include <cassert>

int main() {
  using namespace new_mavlink;
  using namespace new_mavlink::mavlink2_adapter;
  assert(command_allowed(400U)); assert(command_allowed(22U)); assert(command_allowed(21U)); assert(!command_allowed(999U));
  CommandRequest request{}; request.request_id = 44U; request.command_id = 400U; request.deadline_ms = 1000U; request.arg_count = 1U; request.args[0] = 1.0F;
  MavlinkFrame frame{}; assert(encode_command_long(request, 255U, 190U, kPx4SystemId, kPx4ComponentId, frame)); assert(frame.length > 0U && frame.bytes[0] == 0xFDU);
  request.command_id = 999U; assert(!encode_command_long(request, 255U, 190U, kPx4SystemId, kPx4ComponentId, frame));
  mavlink_message_t position{}; mavlink_msg_global_position_int_pack(1U, 1U, &position, 10U, 473977400, 85455900, 500000, 490000, 0, 0, 0, 0U); std::uint8_t raw[280U]{}; const std::uint16_t raw_length = mavlink_msg_to_send_buffer(raw, &position); PositionTelemetry decoded_position{}; assert(decode_global_position(raw, raw_length, decoded_position)); assert(decoded_position.latitude_e7 == 473977400 && decoded_position.altitude_mm == 500000);
  mavlink_message_t attitude{}; mavlink_msg_attitude_pack(1U, 1U, &attitude, 10U, 0.1F, -0.2F, 0.3F, 0.0F, 0.0F, 0.0F); const std::uint16_t attitude_length = mavlink_msg_to_send_buffer(raw, &attitude); AttitudeTelemetry decoded_attitude{}; assert(decode_attitude(raw, attitude_length, decoded_attitude)); assert(decoded_attitude.roll_urad == 100000 && decoded_attitude.pitch_urad == -200000);
  return 0;
}
