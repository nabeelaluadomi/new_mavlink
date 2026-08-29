#include "new_mavlink/crypto.hpp"
#include "new_mavlink/messages.hpp"
#include "new_mavlink/record.hpp"
#include "new_mavlink/transport.hpp"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <utility>

int main(int argc, char** argv) {
  using namespace new_mavlink;
  const std::uint16_t ingress_port = argc > 1 ? static_cast<std::uint16_t>(std::strtoul(argv[1], nullptr, 10)) : 24660U;
  const std::uint16_t egress_port = argc > 2 ? static_cast<std::uint16_t>(std::strtoul(argv[2], nullptr, 10)) : 24661U;
  const std::uint64_t duration_ms = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 0U;
  std::array<std::uint8_t, 16U> psk{}; for (std::size_t i = 0U; i < psk.size(); ++i) psk[i] = static_cast<std::uint8_t>(i + 1U);
  DirectionalKeys vehicle_unused{}; DirectionalKeys proxy_ingress{}; DirectionalKeys proxy_egress{}; DirectionalKeys gcs_unused{};
  if (!derive_directional_keys(psk, SessionId{1U, 1U}, 1U, 3U, vehicle_unused, proxy_ingress) || !derive_directional_keys(psk, SessionId{1U, 1U}, 3U, 2U, proxy_egress, gcs_unused)) return 2;
  Session ingress_session(proxy_ingress); Session egress_session(proxy_egress); UdpEndpoint udp; if (!udp.bind(ingress_port)) return 3;
  std::printf("NEWMAVLINK_PROXY_READY ingress=%u egress=%u duration_ms=%llu session_termination=enabled\n", ingress_port, egress_port, static_cast<unsigned long long>(duration_ms)); std::fflush(stdout);
  NativeEndpoint ingress(std::move(ingress_session)); NativeEndpoint egress(std::move(egress_session)); std::uint64_t egress_sequence = 0U; bool received_any = false; const auto started = std::chrono::steady_clock::now();
  for (;;) {
    const std::uint64_t elapsed = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count());
    OpenRecord opened{}; RecordError error = RecordError::None;
    if (!ingress.receive_uncommitted(udp, static_cast<std::uint32_t>(elapsed), opened, error)) {
      if (error == RecordError::Timeout) { if (duration_ms == 0U) { std::printf("NEWMAVLINK_PROXY_INGRESS_FAIL error=%u\n", static_cast<unsigned>(error)); return 4; } if (elapsed >= duration_ms) break; continue; }
      std::printf("NEWMAVLINK_PROXY_INGRESS_FAIL error=%u\n", static_cast<unsigned>(error)); return 4;
    }
    received_any = true;
    if (!known_type(opened.header.type) || opened.length == 0U) { std::printf("NEWMAVLINK_PROXY_SCHEMA_REJECT=PASS\n"); continue; }
    if (opened.header.type == MessageType::Position) { Position position{}; if (!decode_position(opened.payload.data(), opened.length, position)) { std::printf("NEWMAVLINK_PROXY_SCHEMA_REJECT=PASS\n"); continue; } }
    else if (opened.header.type == MessageType::Attitude) { Attitude attitude{}; if (!decode_attitude(opened.payload.data(), opened.length, attitude)) { std::printf("NEWMAVLINK_PROXY_SCHEMA_REJECT=PASS\n"); continue; } }
    else if (opened.header.type == MessageType::Battery) { Battery battery{}; if (!decode_battery(opened.payload.data(), opened.length, battery)) { std::printf("NEWMAVLINK_PROXY_SCHEMA_REJECT=PASS\n"); continue; } }
    if (!ingress.commit(opened, error)) { std::printf("NEWMAVLINK_PROXY_COMMIT_FAIL error=%u\n", static_cast<unsigned>(error)); return 8; }
    RecordHeader out_header = opened.header; out_header.source_id = 3U; out_header.destination_id = 2U; out_header.sequence = egress_sequence++; out_header.session_id = 1U; out_header.epoch = 1U;
    if (!egress.send(out_header, opened.payload.data(), opened.length, udp, "127.0.0.1", egress_port, error)) { std::printf("NEWMAVLINK_PROXY_EGRESS_FAIL error=%u\n", static_cast<unsigned>(error)); return 5; }
    std::printf("NEWMAVLINK_PROXY_REENCRYPT=PASS type=%u bytes=%zu sequence=%llu\n", static_cast<unsigned>(out_header.type), opened.length, static_cast<unsigned long long>(out_header.sequence)); std::fflush(stdout);
    if (duration_ms == 0U) break;
    if (elapsed >= duration_ms) break;
  }
  if (received_any) std::printf("NEWMAVLINK_PROXY_STREAM=PASS duration_ms=%llu\n", static_cast<unsigned long long>(duration_ms)); return received_any ? 0 : 4;
}
