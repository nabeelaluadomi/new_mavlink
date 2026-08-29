#include "new_mavlink/publisher.hpp"

#include <cassert>

int main() {
  using namespace new_mavlink;
  StatePublisher publisher(1U, 3U, kHelpfulSnapshotPeriodMs);
  Position position{}; position.meta = StateMeta{42U, 0U, 0U, 1U, 0U, 0U}; position.latitude_e7 = 473977400; position.longitude_e7 = 85455900; position.altitude_mm = 500000;
  Attitude attitude{}; attitude.meta = StateMeta{43U, 0U, 0U, 1U, 0U, 0U};
  Battery battery{}; battery.meta = StateMeta{44U, 0U, 0U, 1U, 0U, 0U}; battery.voltage_mv = 16000U; battery.remaining_permille = 900U;
  assert(publisher.update_position(position, 0U)); assert(publisher.update_attitude(attitude, 0U)); assert(publisher.update_battery(battery, 0U));
  PublishedMessage output[3U]{}; assert(publisher.poll(0U, output, 3U) == 3U); assert(output[0].kind == PublishKind::OnChange); assert(output[0].message.header.flags == kFlagDelta);
  assert(publisher.poll(1000U, output, 3U) == 0U);
  assert(!publisher.update_position(position, 2000U)); assert(publisher.poll(2000U, output, 3U) == 0U);
  position.latitude_e7 += 10; assert(publisher.update_position(position, 3000U)); assert(publisher.poll(3000U, output, 3U) == 1U); assert(output[0].kind == PublishKind::OnChange); assert(output[0].message.header.flags == kFlagDelta);
  assert(publisher.poll(9999U, output, 3U) == 0U);
  assert(publisher.poll(10000U, output, 3U) == 3U); for (const auto& item : output) { assert(item.kind == PublishKind::HelpfulSnapshot); assert((item.message.header.flags & kFlagSnapshot) != 0U); }
  assert(publisher.poll(19999U, output, 3U) == 0U); assert(publisher.poll(20000U, output, 3U) == 3U);
  return 0;
}
