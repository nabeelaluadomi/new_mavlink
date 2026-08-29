#include "new_mavlink/qos.hpp"

#include <cassert>

int main() {
  using namespace new_mavlink;
  BoundedQosQueue queue(2U);
  NativeMessage state{}; state.header.type = MessageType::Position; state.header.qos = Qos::StateLatest; state.length = 1U; state.payload[0] = 1U;
  assert(queue.push(state, 0U)); state.payload[0] = 2U; assert(queue.push(state, 0U));
  NativeMessage critical{}; critical.header.type = MessageType::CommandAck; critical.header.qos = Qos::CriticalReliable; critical.length = 1U; assert(queue.push(critical, 0U));
  NativeMessage out{}; assert(queue.pop(out, 0U)); assert(out.header.qos == Qos::CriticalReliable); assert(queue.pop(out, 0U)); assert(out.payload[0] == 2U);
  assert(queue.metrics().dropped_latest >= 1U);
  BoundedQosQueue streams(3U); NativeMessage first{}; first.header.type = MessageType::Position; first.header.qos = Qos::StateLatest; first.header.source_id = 1U; first.length = 16U; StateMeta first_meta{10U, 1U, 0U, 1U, 1U, 0U}; assert(encode_state_meta(first_meta, first.payload.data(), first.payload.size()));
  NativeMessage second = first; StateMeta second_meta{11U, 1U, 0U, 1U, 1U, 0U}; assert(encode_state_meta(second_meta, second.payload.data(), second.payload.size())); assert(streams.push(first, 0U)); assert(streams.push(second, 0U)); assert(streams.size() == 2U);
  BoundedQosQueue typed_streams(3U); NativeMessage typed_first{}; typed_first.header.type = MessageType::Position; typed_first.header.qos = Qos::StateLatest; typed_first.header.source_id = 1U; Position typed_position{}; typed_position.meta = StateMeta{100U, 1U, 0U, 1U, 1U, 0U}; typed_position.latitude_e7 = 1; typed_position.longitude_e7 = 2; assert(encode_position(typed_position, typed_first.payload.data(), typed_first.payload.size())); typed_first.length = kPositionBytes; NativeMessage typed_second = typed_first; typed_second.payload.fill(0U); typed_position.meta.resource_id = 101U; assert(encode_position(typed_position, typed_second.payload.data(), typed_second.payload.size())); typed_second.length = kPositionBytes; assert(typed_streams.push(typed_first, 0U)); assert(typed_streams.push(typed_second, 0U)); assert(typed_streams.size() == 2U);
  return 0;
}
