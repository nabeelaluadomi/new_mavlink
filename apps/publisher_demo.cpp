#include "new_mavlink/publisher.hpp"

#include <array>
#include <cstdio>

int main() {
  using namespace new_mavlink;
  StatePublisher publisher(1U, 3U);
  Position position{}; position.meta = StateMeta{42U, 0U, 0U, 1U, 0U, 0U}; position.latitude_e7 = 473977400; position.longitude_e7 = 85455900; position.altitude_mm = 500000;
  Attitude attitude{}; attitude.meta = StateMeta{43U, 0U, 0U, 1U, 0U, 0U};
  Battery battery{}; battery.meta = StateMeta{44U, 0U, 0U, 1U, 0U, 0U}; battery.voltage_mv = 16000U; battery.remaining_permille = 900U;
  std::array<PublishedMessage, kMaxHelpfulSnapshotMessages> output{};
  for (std::uint64_t now = 0U; now <= 20000U; now += 1000U) {
    if (now == 0U) { publisher.update_position(position, now); publisher.update_attitude(attitude, now); publisher.update_battery(battery, now); }
    if (now == 3000U) { position.latitude_e7 += 10; publisher.update_position(position, now); }
    const std::size_t count = publisher.poll(now, output.data(), output.size());
    for (std::size_t i = 0U; i < count; ++i) {
      const char* kind = output[i].kind == PublishKind::HelpfulSnapshot ? "HELPFUL_SNAPSHOT" : "ON_CHANGE";
      std::printf("NEWMAVLINK_PUBLISH type=%u kind=%s flags=%u length=%zu now_ms=%llu\n", static_cast<unsigned>(output[i].message.header.type), kind, static_cast<unsigned>(output[i].message.header.flags), output[i].message.length, static_cast<unsigned long long>(now));
    }
  }
  return 0;
}
