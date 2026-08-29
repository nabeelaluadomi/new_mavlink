#include "new_mavlink/publisher.hpp"

#include <algorithm>
#include <limits>

namespace new_mavlink {
namespace {

std::uint32_t next_generation(std::uint32_t current) noexcept {
  return current == std::numeric_limits<std::uint32_t>::max() ? current : current + 1U;
}

void prepare_header(NativeMessage& message, MessageType type, std::uint16_t source,
                    std::uint16_t destination, bool snapshot) noexcept {
  message = NativeMessage{};
  message.header.type = type;
  message.header.qos = Qos::StateLatest;
  message.header.source_id = source;
  message.header.destination_id = destination;
  message.header.flags = snapshot ? kFlagSnapshot : kFlagDelta;
}

} // namespace

StatePublisher::StatePublisher(std::uint16_t source_id, std::uint16_t destination_id,
                               std::uint64_t snapshot_period_ms) noexcept
    : source_id_(source_id), destination_id_(destination_id),
      snapshot_period_ms_(snapshot_period_ms == 0U ? kHelpfulSnapshotPeriodMs : snapshot_period_ms) {}

bool StatePublisher::changed_position(const Position& value) const noexcept {
  if (!state_.has_position) return true;
  const Position& old = state_.position;
  return old.meta.resource_id != value.meta.resource_id || old.meta.source_id != value.meta.source_id ||
         old.latitude_e7 != value.latitude_e7 || old.longitude_e7 != value.longitude_e7 ||
         old.altitude_mm != value.altitude_mm;
}

bool StatePublisher::changed_attitude(const Attitude& value) const noexcept {
  if (!state_.has_attitude) return true;
  const Attitude& old = state_.attitude;
  return old.meta.resource_id != value.meta.resource_id || old.meta.source_id != value.meta.source_id ||
         old.roll_urad != value.roll_urad || old.pitch_urad != value.pitch_urad || old.yaw_urad != value.yaw_urad;
}

bool StatePublisher::changed_battery(const Battery& value) const noexcept {
  if (!state_.has_battery) return true;
  const Battery& old = state_.battery;
  return old.meta.resource_id != value.meta.resource_id || old.meta.source_id != value.meta.source_id ||
         old.voltage_mv != value.voltage_mv || old.current_ma != value.current_ma ||
         old.remaining_permille != value.remaining_permille;
}

bool StatePublisher::update_position(const Position& value, std::uint64_t now_ms) noexcept {
  if (!changed_position(value)) return false;
  const std::uint32_t previous = state_.has_position ? position_generation_ : 0U;
  state_.position = value; state_.has_position = true; position_generation_ = next_generation(position_generation_);
  state_.position.meta.source_id = value.meta.source_id == 0U ? source_id_ : value.meta.source_id;
  state_.position.meta.generation = position_generation_; state_.position.meta.base_generation = previous; state_.position.meta.snapshot = 0U;
  position_dirty_ = true; if (next_snapshot_ms_ == 0U) next_snapshot_ms_ = now_ms + snapshot_period_ms_; return true;
}

bool StatePublisher::update_attitude(const Attitude& value, std::uint64_t now_ms) noexcept {
  if (!changed_attitude(value)) return false;
  const std::uint32_t previous = state_.has_attitude ? attitude_generation_ : 0U;
  state_.attitude = value; state_.has_attitude = true; attitude_generation_ = next_generation(attitude_generation_);
  state_.attitude.meta.source_id = value.meta.source_id == 0U ? source_id_ : value.meta.source_id;
  state_.attitude.meta.generation = attitude_generation_; state_.attitude.meta.base_generation = previous; state_.attitude.meta.snapshot = 0U;
  attitude_dirty_ = true; if (next_snapshot_ms_ == 0U) next_snapshot_ms_ = now_ms + snapshot_period_ms_; return true;
}

bool StatePublisher::update_battery(const Battery& value, std::uint64_t now_ms) noexcept {
  if (!changed_battery(value)) return false;
  const std::uint32_t previous = state_.has_battery ? battery_generation_ : 0U;
  state_.battery = value; state_.has_battery = true; battery_generation_ = next_generation(battery_generation_);
  state_.battery.meta.source_id = value.meta.source_id == 0U ? source_id_ : value.meta.source_id;
  state_.battery.meta.generation = battery_generation_; state_.battery.meta.base_generation = previous; state_.battery.meta.snapshot = 0U;
  battery_dirty_ = true; if (next_snapshot_ms_ == 0U) next_snapshot_ms_ = now_ms + snapshot_period_ms_; return true;
}

bool StatePublisher::build_position(PublishedMessage& output, bool snapshot) noexcept {
  if (!state_.has_position) return false; prepare_header(output.message, MessageType::Position, source_id_, destination_id_, snapshot);
  Position value = state_.position; value.meta.snapshot = snapshot ? 1U : 0U; value.meta.base_generation = snapshot ? 0U : value.meta.base_generation;
  if (!encode_position(value, output.message.payload.data(), output.message.payload.size())) return false;
  output.message.length = kPositionBytes; output.kind = snapshot ? PublishKind::HelpfulSnapshot : PublishKind::OnChange; return true;
}

bool StatePublisher::build_attitude(PublishedMessage& output, bool snapshot) noexcept {
  if (!state_.has_attitude) return false; prepare_header(output.message, MessageType::Attitude, source_id_, destination_id_, snapshot);
  Attitude value = state_.attitude; value.meta.snapshot = snapshot ? 1U : 0U; value.meta.base_generation = snapshot ? 0U : value.meta.base_generation;
  if (!encode_attitude(value, output.message.payload.data(), output.message.payload.size())) return false;
  output.message.length = kAttitudeBytes; output.kind = snapshot ? PublishKind::HelpfulSnapshot : PublishKind::OnChange; return true;
}

bool StatePublisher::build_battery(PublishedMessage& output, bool snapshot) noexcept {
  if (!state_.has_battery) return false; prepare_header(output.message, MessageType::Battery, source_id_, destination_id_, snapshot);
  Battery value = state_.battery; value.meta.snapshot = snapshot ? 1U : 0U; value.meta.base_generation = snapshot ? 0U : value.meta.base_generation;
  if (!encode_battery(value, output.message.payload.data(), output.message.payload.size())) return false;
  output.message.length = kBatteryBytes; output.kind = snapshot ? PublishKind::HelpfulSnapshot : PublishKind::OnChange; return true;
}

void StatePublisher::schedule_next_snapshot(std::uint64_t now_ms) noexcept {
  const std::uint64_t period = snapshot_period_ms_;
  if (now_ms > std::numeric_limits<std::uint64_t>::max() - period) next_snapshot_ms_ = std::numeric_limits<std::uint64_t>::max();
  else next_snapshot_ms_ = now_ms + period;
}

std::size_t StatePublisher::poll(std::uint64_t now_ms, PublishedMessage* output,
                                 std::size_t output_capacity) noexcept {
  if (output == nullptr || output_capacity == 0U) return 0U;
  std::size_t count = 0U;
  const bool due = next_snapshot_ms_ != 0U && now_ms >= next_snapshot_ms_;
  if (due) {
    if (count < output_capacity && build_position(output[count], true)) ++count;
    if (count < output_capacity && build_attitude(output[count], true)) ++count;
    if (count < output_capacity && build_battery(output[count], true)) ++count;
    position_dirty_ = attitude_dirty_ = battery_dirty_ = false; schedule_next_snapshot(now_ms); return count;
  }
  if (position_dirty_ && count < output_capacity && build_position(output[count], false)) { position_dirty_ = false; ++count; }
  if (attitude_dirty_ && count < output_capacity && build_attitude(output[count], false)) { attitude_dirty_ = false; ++count; }
  if (battery_dirty_ && count < output_capacity && build_battery(output[count], false)) { battery_dirty_ = false; ++count; }
  return count;
}

} // namespace new_mavlink
