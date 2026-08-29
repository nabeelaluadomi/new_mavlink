#pragma once

#include "new_mavlink/messages.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_map>

namespace new_mavlink {

struct QueueMetrics {
  std::uint64_t enqueued{0U};
  std::uint64_t delivered{0U};
  std::uint64_t dropped_latest{0U};
  std::uint64_t rejected_critical{0U};
  std::uint64_t expired{0U};
};

class BoundedQosQueue {
 public:
  explicit BoundedQosQueue(std::size_t capacity) noexcept : capacity_(capacity) {}
  bool push(NativeMessage message, std::uint64_t now_ms) noexcept;
  bool pop(NativeMessage& message, std::uint64_t now_ms) noexcept;
  void clear() noexcept;
  std::size_t size() const noexcept { return critical_.size() + state_.size() + events_.size(); }
  const QueueMetrics& metrics() const noexcept { return metrics_; }

 private:
  bool expired(const NativeMessage& message, std::uint64_t now_ms) const noexcept;
  bool is_state(const NativeMessage& message) const noexcept;
  std::uint64_t stream_key(const NativeMessage& message) const noexcept;
  std::size_t capacity_{0U};
  std::deque<NativeMessage> critical_{};
  std::deque<NativeMessage> events_{};
  std::unordered_map<std::uint64_t, NativeMessage> state_{};
  QueueMetrics metrics_{};
};

} // namespace new_mavlink
