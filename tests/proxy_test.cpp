#include "new_mavlink/proxy.hpp"
#include "new_mavlink/crypto.hpp"

#include <array>
#include <cassert>

int main() {
  using namespace new_mavlink;
  std::array<std::uint8_t, 16U> key{}; for (std::size_t i = 0U; i < key.size(); ++i) key[i] = static_cast<std::uint8_t>(i + 3U);
  DirectionalKeys a_keys{}; DirectionalKeys b_keys{}; SessionId sid{9U, 1U}; assert(derive_directional_keys(key, sid, 1U, 2U, a_keys, b_keys));
  Session a(a_keys); Session b(b_keys); NativeProxy proxy(4U); assert(proxy.add_link(1U, a)); assert(proxy.add_link(2U, b));
  Subscription subscription{MessageType::Position, Qos::StateLatest, 2U, 0U, 0U}; assert(proxy.subscribe(2U, subscription));
  NativeMessage incoming{}; incoming.header.type = MessageType::Position; incoming.header.qos = Qos::StateLatest; incoming.header.source_id = 2U; incoming.header.destination_id = 1U; incoming.header.session_id = 9U; incoming.header.epoch = 1U; incoming.length = kPositionBytes; Position position{}; position.meta = StateMeta{42U, 1U, 0U, 1U, 1U, 0U}; position.latitude_e7 = 473977400; position.longitude_e7 = 85455900; position.altitude_mm = 500000; assert(encode_position(position, incoming.payload.data(), incoming.payload.size()));
  OpenRecord opened{}; opened.header = incoming.header; opened.length = incoming.length; std::copy(incoming.payload.begin(), incoming.payload.begin() + static_cast<std::ptrdiff_t>(incoming.length), opened.payload.begin());
  assert(proxy.ingest(1U, opened, 0U)); NativeMessage out{}; assert(proxy.next(2U, out, 0U)); assert(out.header.type == MessageType::Position); assert(proxy.cached(MessageType::Position, out));
  Position gap_position = position; gap_position.meta = StateMeta{42U, 3U, 99U, 1U, 0U, 0U}; assert(encode_position(gap_position, opened.payload.data(), opened.payload.size())); opened.length = kPositionBytes; assert(!proxy.ingest(1U, opened, 0U)); ResyncRequest request{}; assert(proxy.next_resync(request)); assert(request.type == MessageType::Position && request.resource_id == 42U);
  return 0;
}
