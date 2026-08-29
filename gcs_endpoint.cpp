#include "new_mavlink/messages.hpp"
#include "new_mavlink/crypto.hpp"
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
  const std::uint16_t listen_port = argc > 1 ? static_cast<std::uint16_t>(std::strtoul(argv[1], nullptr, 10)) : 24661U;
  const std::uint64_t duration_ms = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 0U;
  std::array<std::uint8_t, 16U> psk{}; for (std::size_t i = 0U; i < psk.size(); ++i) psk[i] = static_cast<std::uint8_t>(i + 1U);
  DirectionalKeys unused{}; DirectionalKeys rx{}; if (!derive_directional_keys(psk, SessionId{1U, 1U}, 3U, 2U, unused, rx)) return 2;
  Session session(rx); UdpEndpoint udp; if (!udp.bind(listen_port)) return 3;
  std::printf("NEWMAVLINK_GCS_READY native_udp=%u duration_ms=%llu\n", listen_port, static_cast<unsigned long long>(duration_ms)); std::fflush(stdout);
  NativeEndpoint endpoint(std::move(session)); std::size_t received_count = 0U; const auto started = std::chrono::steady_clock::now();
  for (;;) {
    const std::uint64_t elapsed = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count());
    OpenRecord opened{}; RecordError error = RecordError::None;
    if (!endpoint.receive_uncommitted(udp, static_cast<std::uint32_t>(elapsed), opened, error)) {
      if (error == RecordError::Timeout && duration_ms > 0U && elapsed < duration_ms) { continue; }
      if (error == RecordError::Timeout && duration_ms == 0U) std::printf("NEWMAVLINK_GCS_RECEIVE_FAIL error=%u\n", static_cast<unsigned>(error));
      return received_count == 0U ? 4 : 0;
    }
    bool valid = false;
    if (opened.header.type == MessageType::Position) { Position value{}; valid = decode_position(opened.payload.data(), opened.length, value); if (valid) std::printf("NEWMAVLINK_GCS_STATE=PASS type=Position generation=%u lat=%d lon=%d alt_mm=%d\n", value.meta.generation, value.latitude_e7, value.longitude_e7, value.altitude_mm); }
    else if (opened.header.type == MessageType::Attitude) { Attitude value{}; valid = decode_attitude(opened.payload.data(), opened.length, value); if (valid) std::printf("NEWMAVLINK_GCS_STATE=PASS type=Attitude generation=%u roll=%d pitch=%d yaw=%d\n", value.meta.generation, value.roll_urad, value.pitch_urad, value.yaw_urad); }
    else if (opened.header.type == MessageType::Battery) { Battery value{}; valid = decode_battery(opened.payload.data(), opened.length, value); if (valid) std::printf("NEWMAVLINK_GCS_STATE=PASS type=Battery generation=%u voltage_mv=%u remaining_permille=%u\n", value.meta.generation, value.voltage_mv, value.remaining_permille); }
    if (!valid) { std::printf("NEWMAVLINK_GCS_SCHEMA_REJECT=PASS type=%u\n", static_cast<unsigned>(opened.header.type)); std::fflush(stdout); continue; }
    if (!endpoint.commit(opened, error)) { std::printf("NEWMAVLINK_GCS_COMMIT_FAIL error=%u\n", static_cast<unsigned>(error)); return 6; }
    ++received_count; std::fflush(stdout);
    if (duration_ms == 0U) { std::printf("NEWMAVLINK_GCS_SNAPSHOT=PASS count=%zu\n", received_count); return 0; }
    if (elapsed >= duration_ms) break;
  }
  std::printf("NEWMAVLINK_GCS_STREAM=PASS count=%zu duration_ms=%llu\n", received_count, static_cast<unsigned long long>(duration_ms)); return received_count > 0U ? 0 : 4;
}
