#include "new_mavlink/qos.hpp"

#include <iterator>

namespace new_mavlink {

bool BoundedQosQueue::expired(const NativeMessage& message, std::uint64_t now_ms) const noexcept {
  return message.header.deadline_ms != 0U && now_ms > message.header.deadline_ms;
}

bool BoundedQosQueue::is_state(const NativeMessage& message) const noexcept {
  return state_type(message.header.type) && message.header.qos == Qos::StateLatest;
}

std::uint64_t BoundedQosQueue::stream_key(const NativeMessage& message) const noexcept {
  std::uint32_t resource = 0U;
  std::uint16_t source = message.header.source_id;
  StateMeta meta{};
  if (message.length >= 16U && decode_state_meta(message.payload.data(), 16U, meta)) {
    resource = meta.resource_id; source = meta.source_id;
  }
  return (static_cast<std::uint64_t>(static_cast<std::uint8_t>(message.header.type)) << 56U) |
         (static_cast<std::uint64_t>(source) << 32U) | resource;
}

bool BoundedQosQueue::push(NativeMessage message, std::uint64_t now_ms) noexcept {
  if (capacity_ == 0U || message.length > kMaxPayloadBytes || expired(message, now_ms)) {
    ++metrics_.expired; return false;
  }
  if (is_state(message)) {
    const auto key = stream_key(message);
    const auto it = state_.find(key);
    if (it != state_.end()) { it->second = message; ++metrics_.dropped_latest; ++metrics_.enqueued; return true; }
    while (size() >= capacity_) {
      if (!events_.empty()) { events_.pop_front(); ++metrics_.dropped_latest; }
      else break;
    }
    if (size() >= capacity_) { ++metrics_.dropped_latest; return false; }
    state_.emplace(key, message); ++metrics_.enqueued; return true;
  }
  const bool critical = message.header.qos == Qos::CriticalReliable || message.header.qos == Qos::CommandReliable;
  if (size() >= capacity_) {
    if (critical) { ++metrics_.rejected_critical; return false; }
    if (!events_.empty()) { events_.pop_front(); ++metrics_.dropped_latest; }
    else { ++metrics_.dropped_latest; return false; }
  }
  (critical ? critical_ : events_).push_back(message); ++metrics_.enqueued; return true;
}

bool BoundedQosQueue::pop(NativeMessage& message, std::uint64_t now_ms) noexcept {
  while (true) {
    if (!critical_.empty()) { message = critical_.front(); critical_.pop_front(); }
    else if (!events_.empty()) { message = events_.front(); events_.pop_front(); }
    else if (!state_.empty()) { auto it = state_.begin(); for (auto candidate = std::next(state_.begin()); candidate != state_.end(); ++candidate) if (candidate->first < it->first) it = candidate; message = it->second; state_.erase(it); }
    else return false;
    if (expired(message, now_ms)) { ++metrics_.expired; continue; }
    ++metrics_.delivered; return true;
  }
}

void BoundedQosQueue::clear() noexcept {
  critical_.clear(); events_.clear(); state_.clear();
}

} // namespace new_mavlink
