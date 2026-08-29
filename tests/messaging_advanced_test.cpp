#include "new_mavlink/proxy.hpp"
#include "new_mavlink/crypto.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>

namespace {

new_mavlink::OpenRecord make_position(std::uint32_t generation, std::uint32_t base_generation, bool snapshot) {
  using namespace new_mavlink;
  OpenRecord record{}; record.header.type = MessageType::Position; record.header.qos = Qos::StateLatest; record.header.source_id = 2U; record.header.destination_id = 1U; record.header.session_id = 9U; record.header.epoch = 1U;
  Position position{}; position.meta = StateMeta{42U, generation, base_generation, 2U, static_cast<std::uint8_t>(snapshot ? 1U : 0U), 0U}; position.latitude_e7 = 473977400 + static_cast<std::int32_t>(generation); position.longitude_e7 = 85455900; position.altitude_mm = 500000; assert(encode_position(position, record.payload.data(), record.payload.size())); record.length = kPositionBytes; return record;
}

} // namespace

int main() {
  using namespace new_mavlink;
  std::array<std::uint8_t, 16U> psk{}; for (std::size_t i = 0U; i < psk.size(); ++i) psk[i] = static_cast<std::uint8_t>(i + 3U);
  DirectionalKeys ingress_keys{}; DirectionalKeys egress_keys{}; assert(derive_directional_keys(psk, SessionId{9U, 1U}, 1U, 2U, ingress_keys, egress_keys)); Session ingress(ingress_keys); Session egress(egress_keys);
  NativeProxy proxy(4U); assert(proxy.add_link(1U, ingress)); assert(proxy.add_link(2U, egress)); Subscription sub{MessageType::Position, Qos::StateLatest, 2U, 0U, 0U}; assert(proxy.subscribe(2U, sub));
  OpenRecord snapshot = make_position(1U, 0U, true); assert(proxy.ingest(1U, snapshot, 0U)); NativeMessage output{}; assert(proxy.next(2U, output, 0U)); Position decoded{}; assert(decode_position(output.payload.data(), output.length, decoded)); assert(decoded.meta.generation == 1U);
  OpenRecord delta = make_position(2U, 1U, false); assert(proxy.ingest(1U, delta, 0U)); assert(proxy.next(2U, output, 0U)); assert(decode_position(output.payload.data(), output.length, decoded)); assert(decoded.meta.generation == 2U);
  OpenRecord reordered = make_position(1U, 0U, true); assert(proxy.ingest(1U, reordered, 0U)); assert(!proxy.next(2U, output, 0U));
  OpenRecord gap = make_position(4U, 1U, false); assert(!proxy.ingest(1U, gap, 0U)); ResyncRequest request{}; assert(proxy.next_resync(request)); assert(request.type == MessageType::Position && request.source_id == 2U && request.resource_id == 42U);
  assert(proxy.unsubscribe(2U, MessageType::Position)); OpenRecord later = make_position(3U, 2U, false); assert(proxy.ingest(1U, later, 0U)); assert(!proxy.next(2U, output, 0U));
  return 0;
}
