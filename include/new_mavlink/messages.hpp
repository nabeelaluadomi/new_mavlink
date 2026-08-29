#pragma once

#include "new_mavlink/wire.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace new_mavlink {

constexpr std::size_t kMaxArgs = 7U;
constexpr std::size_t kCommandBytes = 16U + 4U * kMaxArgs;
constexpr std::size_t kAckBytes = 16U;
constexpr std::size_t kSubscribeBytes = 10U;
constexpr std::size_t kResyncBytes = 8U;
constexpr std::size_t kPositionBytes = 28U;
constexpr std::size_t kAttitudeBytes = 28U;
constexpr std::size_t kBatteryBytes = 24U;
constexpr std::size_t kHeartbeatBytes = 16U;

struct CommandRequest {
  std::uint64_t request_id{0U};
  std::uint16_t command_id{0U};
  std::uint16_t deadline_ms{0U};
  std::uint8_t arg_count{0U};
  std::array<float, kMaxArgs> args{};
};

struct CommandAck {
  std::uint64_t request_id{0U};
  std::uint8_t status{0U};
  std::uint8_t detail{0U};
  std::uint16_t reserved{0U};
  std::uint32_t result_code{0U};
};

struct Subscription {
  MessageType type{MessageType::Position};
  Qos qos{Qos::StateLatest};
  std::uint16_t source_id{0U};
  std::uint16_t interval_ms{0U};
  std::uint32_t flags{0U};
};

struct ResyncRequest {
  MessageType type{MessageType::Position};
  std::uint16_t source_id{0U};
  std::uint32_t resource_id{0U};
};

struct StateMeta {
  std::uint32_t resource_id{0U};
  std::uint32_t generation{0U};
  std::uint32_t base_generation{0U};
  std::uint16_t source_id{0U};
  std::uint8_t snapshot{0U};
  std::uint8_t reserved{0U};
};

struct Position {
  StateMeta meta{};
  std::int32_t latitude_e7{0};
  std::int32_t longitude_e7{0};
  std::int32_t altitude_mm{0};
};

struct Attitude {
  StateMeta meta{};
  std::int32_t roll_urad{0};
  std::int32_t pitch_urad{0};
  std::int32_t yaw_urad{0};
};

struct Battery {
  StateMeta meta{};
  std::uint16_t voltage_mv{0U};
  std::uint16_t current_ma{0U};
  std::uint16_t remaining_permille{0U};
  std::uint16_t reserved{0U};
};

struct Heartbeat {
  std::uint64_t uptime_ms{0U};
  std::uint32_t capabilities{0U};
  std::uint32_t state{0U};
};

struct NativeMessage {
  RecordHeader header{};
  std::array<std::uint8_t, kMaxPayloadBytes> payload{};
  std::size_t length{0U};
};

bool encode_command(const CommandRequest&, std::uint8_t*, std::size_t,
                    std::size_t&) noexcept;
bool decode_command(const std::uint8_t*, std::size_t, CommandRequest&) noexcept;
bool encode_ack(const CommandAck&, std::uint8_t*, std::size_t,
                std::size_t&) noexcept;
bool decode_ack(const std::uint8_t*, std::size_t, CommandAck&) noexcept;
bool encode_subscription(const Subscription&, std::uint8_t*, std::size_t,
                         std::size_t&) noexcept;
bool decode_subscription(const std::uint8_t*, std::size_t, Subscription&) noexcept;
bool encode_unsubscribe(const Subscription&, std::uint8_t*, std::size_t, std::size_t&) noexcept;
bool decode_unsubscribe(const std::uint8_t*, std::size_t, Subscription&) noexcept;
bool encode_resync(const ResyncRequest&, std::uint8_t*, std::size_t,
                   std::size_t&) noexcept;
bool decode_resync(const std::uint8_t*, std::size_t, ResyncRequest&) noexcept;
bool encode_state_meta(const StateMeta&, std::uint8_t*, std::size_t) noexcept;
bool decode_state_meta(const std::uint8_t*, std::size_t, StateMeta&) noexcept;
bool encode_position(const Position&, std::uint8_t*, std::size_t) noexcept;
bool decode_position(const std::uint8_t*, std::size_t, Position&) noexcept;
bool encode_attitude(const Attitude&, std::uint8_t*, std::size_t) noexcept;
bool decode_attitude(const std::uint8_t*, std::size_t, Attitude&) noexcept;
bool encode_battery(const Battery&, std::uint8_t*, std::size_t) noexcept;
bool decode_battery(const std::uint8_t*, std::size_t, Battery&) noexcept;
bool encode_heartbeat(const Heartbeat&, std::uint8_t*, std::size_t) noexcept;
bool decode_heartbeat(const std::uint8_t*, std::size_t, Heartbeat&) noexcept;

} // namespace new_mavlink
