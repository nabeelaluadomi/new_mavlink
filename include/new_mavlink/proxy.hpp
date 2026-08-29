#pragma once

#include "new_mavlink/qos.hpp"
#include "new_mavlink/record.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

namespace new_mavlink {

constexpr std::size_t kMaxProxySubscriptions = 64U;

struct ProxyMetrics {
  std::uint64_t ingress_accepted{0U};
  std::uint64_t ingress_rejected{0U};
  std::uint64_t forwarded{0U};
  std::uint64_t cache_updates{0U};
  std::uint64_t resync_requests{0U};
};

struct ProxyLink {
  std::uint16_t link_id{0U};
  Session* session{nullptr};
  BoundedQosQueue queue{64U};
  bool active{false};
};

class NativeProxy {
 public:
  explicit NativeProxy(std::size_t queue_capacity = 64U) noexcept : queue_capacity_(queue_capacity) {}
  bool add_link(std::uint16_t link_id, Session& session) noexcept;
  bool remove_link(std::uint16_t link_id) noexcept;
  bool subscribe(std::uint16_t link_id, const Subscription& subscription) noexcept;
  bool unsubscribe(std::uint16_t link_id, MessageType type) noexcept;
  bool ingest(std::uint16_t ingress_link, const OpenRecord& opened,
              std::uint64_t now_ms) noexcept;
  bool next(std::uint16_t egress_link, NativeMessage& message,
            std::uint64_t now_ms) noexcept;
  bool cached(MessageType type, NativeMessage& message) const noexcept;
  bool next_resync(ResyncRequest& request) noexcept;
  std::size_t link_count() const noexcept { return links_.size(); }
  const ProxyMetrics& metrics() const noexcept { return metrics_; }

 private:
  struct LinkState {
    Session* session{nullptr};
    BoundedQosQueue queue{64U};
    std::unordered_map<std::uint8_t, Subscription> subscriptions{};
  };
  std::size_t queue_capacity_{64U};
  std::unordered_map<std::uint16_t, LinkState> links_{};
  std::unordered_map<std::uint64_t, NativeMessage> cache_{};
  std::deque<ResyncRequest> resync_{};
  ProxyMetrics metrics_{};
};

} // namespace new_mavlink
