#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace new_mavlink {

constexpr std::uint16_t kMagic = 0x4E4DU; // "NM" little-endian
constexpr std::uint8_t kVersion = 1U;
constexpr std::size_t kHeaderBytes = 40U;
constexpr std::size_t kKeyBytes = 16U;
constexpr std::size_t kNonceBytes = 16U;
constexpr std::size_t kTagBytes = 16U;
constexpr std::size_t kMaxPayloadBytes = 768U;
constexpr std::size_t kMaxRecordBytes = kHeaderBytes + kMaxPayloadBytes + kTagBytes;

constexpr std::uint8_t kFlagDelta = 0x01U;
constexpr std::uint8_t kFlagAck = 0x02U;
constexpr std::uint8_t kFlagSnapshot = 0x04U;
constexpr std::uint8_t kFlagResync = 0x08U;
constexpr std::uint8_t kKnownFlags = kFlagDelta | kFlagAck | kFlagSnapshot | kFlagResync;

enum class MessageType : std::uint8_t {
  Hello = 1U,
  Accept = 2U,
  Finish = 3U,
  Subscribe = 4U,
  Resync = 5U,
  Unsubscribe = 6U,
  Heartbeat = 16U,
  VehicleState = 17U,
  Position = 18U,
  Attitude = 19U,
  Battery = 20U,
  LinkStats = 21U,
  SafetyEvent = 22U,
  FailsafeEvent = 23U,
  GeofenceEvent = 24U,
  CommandRequest = 48U,
  CommandAck = 49U,
  CommandResult = 50U,
  SchemaManifest = 64U,
  RateReport = 65U,
  DiagnosticEvent = 66U,
};

enum class Qos : std::uint8_t {
  CriticalReliable = 0U,
  CommandReliable = 1U,
  StateLatest = 2U,
  TelemetryBestEffort = 3U,
  Liveness = 4U,
  Explicit = 5U,
};

struct SessionId {
  std::uint64_t value{0U};
  std::uint64_t epoch{0U};
};

struct DirectionalKeys {
  SessionId session{};
  std::uint16_t local_id{0U};
  std::uint16_t peer_id{0U};
  std::array<std::uint8_t, kKeyBytes> tx{};
  std::array<std::uint8_t, kKeyBytes> rx{};
};

struct RecordHeader {
  std::uint8_t version{kVersion};
  std::uint8_t flags{0U};
  MessageType type{MessageType::Heartbeat};
  Qos qos{Qos::TelemetryBestEffort};
  std::uint16_t source_id{0U};
  std::uint16_t destination_id{0U};
  std::uint64_t session_id{0U};
  std::uint64_t epoch{0U};
  std::uint64_t sequence{0U};
  std::uint16_t payload_len{0U};
  std::uint16_t deadline_ms{0U};
  std::uint16_t header_crc16{0U};
};

bool known_type(MessageType type) noexcept;
bool state_type(MessageType type) noexcept;
bool command_type(MessageType type) noexcept;
bool control_type(MessageType type) noexcept;
bool critical_type(MessageType type) noexcept;
Qos default_qos(MessageType type) noexcept;

} // namespace new_mavlink
