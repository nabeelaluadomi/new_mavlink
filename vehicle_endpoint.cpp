#include "new_mavlink/messages.hpp"
#include "new_mavlink/crypto.hpp"
#include "new_mavlink/publisher.hpp"
#include "new_mavlink/session.hpp"
#include "new_mavlink/transport.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <utility>

int main(int argc, char** argv) {
  using namespace new_mavlink;
  const std::uint16_t listen_port = argc > 1 ? static_cast<std::uint16_t>(std::strtoul(argv[1], nullptr, 10)) : 0U;
  const std::uint16_t peer_port = argc > 2 ? static_cast<std::uint16_t>(std::strtoul(argv[2], nullptr, 10)) : 24660U;
  const std::uint64_t duration_ms = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 0U;
  std::array<std::uint8_t, 16U> psk{}; for (std::size_t i = 0U; i < psk.size(); ++i) psk[i] = static_cast<std::uint8_t>(i + 1U);
  DirectionalKeys tx{}; DirectionalKeys unused{}; if (!derive_directional_keys(psk, SessionId{1U, 1U}, 1U, 3U, tx, unused)) return 2;
  Session session(tx); UdpEndpoint udp; if (!udp.bind(listen_port)) return 3;
  std::printf("NEWMAVLINK_VEHICLE_READY source_udp=%u proxy_udp=%u duration_ms=%llu\n", listen_port, peer_port, static_cast<unsigned long long>(duration_ms)); std::fflush(stdout);
  StatePublisher publisher(1U, 3U); Position position{}; position.meta = StateMeta{42U, 0U, 0U, 1U, 0U, 0U}; position.latitude_e7 = 473977400; position.longitude_e7 = 85455900; position.altitude_mm = 500000; Attitude attitude{}; attitude.meta = StateMeta{43U, 0U, 0U, 1U, 0U, 0U}; Battery battery{}; battery.meta = StateMeta{44U, 0U, 0U, 1U, 0U, 0U}; battery.voltage_mv = 16000U; battery.remaining_permille = 900U;
  NativeEndpoint endpoint(std::move(session)); RecordError error = RecordError::None; std::uint64_t sequence = 0U;
  const std::uint64_t end_ms = duration_ms == 0U ? 0U : duration_ms;
  for (std::uint64_t now_ms = 0U;; now_ms += 100U) {
    if (now_ms == 0U) { publisher.update_position(position, now_ms); publisher.update_attitude(attitude, now_ms); publisher.update_battery(battery, now_ms); }
    if (duration_ms > 0U && now_ms == 3000U) { position.latitude_e7 += 10; publisher.update_position(position, now_ms); }
    PublishedMessage messages[kMaxHelpfulSnapshotMessages]{}; const std::size_t count = publisher.poll(now_ms, messages, kMaxHelpfulSnapshotMessages);
    for (std::size_t i = 0U; i < count; ++i) { RecordHeader header = messages[i].message.header; header.session_id = 1U; header.epoch = 1U; header.sequence = sequence++; header.payload_len = static_cast<std::uint16_t>(messages[i].message.length); if (!endpoint.send(header, messages[i].message.payload.data(), messages[i].message.length, udp, "127.0.0.1", peer_port, error)) { std::printf("NEWMAVLINK_VEHICLE_SEND_FAIL error=%u\n", static_cast<unsigned>(error)); return 5; } const char* kind = messages[i].kind == PublishKind::HelpfulSnapshot ? "HELPFUL_SNAPSHOT" : "ON_CHANGE"; std::printf("NEWMAVLINK_VEHICLE_PUBLISH kind=%s type=%u now_ms=%llu\n", kind, static_cast<unsigned>(header.type), static_cast<unsigned long long>(now_ms)); std::fflush(stdout); }
    if (duration_ms == 0U) { std::printf("NEWMAVLINK_VEHICLE_SNAPSHOT=PASS generation=1\n"); return 0; }
    if (now_ms >= end_ms) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  std::printf("NEWMAVLINK_VEHICLE_STREAM=PASS duration_ms=%llu\n", static_cast<unsigned long long>(duration_ms)); return 0;
}
