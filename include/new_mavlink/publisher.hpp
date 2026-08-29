#pragma once

#include "new_mavlink/messages.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace new_mavlink {

constexpr std::uint64_t kHelpfulSnapshotPeriodMs = 10000U;
constexpr std::size_t kMaxHelpfulSnapshotMessages = 3U;

enum class PublishKind : std::uint8_t {
  OnChange = 0U,
  HelpfulSnapshot = 1U,
};

struct PublishedMessage {
  NativeMessage message{};
  PublishKind kind{PublishKind::OnChange};
};

struct HelpfulState {
  bool has_position{false};
  bool has_attitude{false};
  bool has_battery{false};
  Position position{};
  Attitude attitude{};
  Battery battery{};
};

class StatePublisher final {
 public:
  explicit StatePublisher(std::uint16_t source_id, std::uint16_t destination_id,
                          std::uint64_t snapshot_period_ms = kHelpfulSnapshotPeriodMs) noexcept;

  bool update_position(const Position& value, std::uint64_t now_ms) noexcept;
  bool update_attitude(const Attitude& value, std::uint64_t now_ms) noexcept;
  bool update_battery(const Battery& value, std::uint64_t now_ms) noexcept;

  std::size_t poll(std::uint64_t now_ms, PublishedMessage* output,
                   std::size_t output_capacity) noexcept;
  const HelpfulState& state() const noexcept { return state_; }
  std::uint64_t next_snapshot_ms() const noexcept { return next_snapshot_ms_; }

 private:
  bool changed_position(const Position& value) const noexcept;
  bool changed_attitude(const Attitude& value) const noexcept;
  bool changed_battery(const Battery& value) const noexcept;
  bool build_position(PublishedMessage& output, bool snapshot) noexcept;
  bool build_attitude(PublishedMessage& output, bool snapshot) noexcept;
  bool build_battery(PublishedMessage& output, bool snapshot) noexcept;
  void schedule_next_snapshot(std::uint64_t now_ms) noexcept;

  std::uint16_t source_id_{0U};
  std::uint16_t destination_id_{0U};
  std::uint64_t snapshot_period_ms_{kHelpfulSnapshotPeriodMs};
  std::uint64_t next_snapshot_ms_{0U};
  HelpfulState state_{};
  std::uint32_t position_generation_{0U};
  std::uint32_t attitude_generation_{0U};
  std::uint32_t battery_generation_{0U};
  bool position_dirty_{false};
  bool attitude_dirty_{false};
  bool battery_dirty_{false};
};

} // namespace new_mavlink
