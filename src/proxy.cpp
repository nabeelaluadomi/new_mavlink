#include "new_mavlink/proxy.hpp"

#include <algorithm>

namespace new_mavlink {
namespace {

std::uint64_t stream_key(const NativeMessage& message) noexcept {
  std::uint32_t resource = 0U;
  std::uint16_t source = message.header.source_id;
  StateMeta meta{};
  if (message.length >= 16U && decode_state_meta(message.payload.data(), 16U, meta)) {
    source = meta.source_id; resource = meta.resource_id;
  }
  return (static_cast<std::uint64_t>(static_cast<std::uint8_t>(message.header.type)) << 56U) |
         (static_cast<std::uint64_t>(source) << 32U) | resource;
}

} // namespace

bool NativeProxy::add_link(std::uint16_t link_id, Session& session) noexcept {
  if (link_id == 0U || links_.find(link_id) != links_.end() || links_.size() >= 16U) return false;
  LinkState state{}; state.session = &session; state.queue = BoundedQosQueue(queue_capacity_); links_.emplace(link_id, std::move(state)); return true;
}

bool NativeProxy::remove_link(std::uint16_t link_id) noexcept { return links_.erase(link_id) == 1U; }

bool NativeProxy::subscribe(std::uint16_t link_id, const Subscription& subscription) noexcept {
  const auto it = links_.find(link_id);
  if (it == links_.end() || subscription.source_id == 0U || !known_type(subscription.type) ||
      control_type(subscription.type) || command_type(subscription.type) ||
      static_cast<std::uint8_t>(subscription.qos) > static_cast<std::uint8_t>(Qos::Explicit)) return false;
  const auto key = static_cast<std::uint8_t>(subscription.type);
  if (it->second.subscriptions.find(key) == it->second.subscriptions.end()) {
    std::size_t count = 0U; for (const auto& link : links_) count += link.second.subscriptions.size();
    if (count >= kMaxProxySubscriptions) return false;
  }
  it->second.subscriptions[key] = subscription;
  for (const auto& cached : cache_) {
    if (static_cast<std::uint8_t>(cached.second.header.type) == key && cached.second.header.source_id == subscription.source_id) {
      if (!it->second.queue.push(cached.second, 0U)) return false;
    }
  }
  return true;
}

bool NativeProxy::unsubscribe(std::uint16_t link_id, MessageType type) noexcept {
  const auto it = links_.find(link_id); if (it == links_.end()) return false;
  return it->second.subscriptions.erase(static_cast<std::uint8_t>(type)) == 1U;
}

bool NativeProxy::ingest(std::uint16_t ingress_link, const OpenRecord& opened, std::uint64_t now_ms) noexcept {
  const auto ingress = links_.find(ingress_link);
  if (ingress == links_.end() || ingress->second.session == nullptr || opened.length > kMaxPayloadBytes || !known_type(opened.header.type) ||
      opened.header.source_id != ingress->second.session->keys().peer_id || opened.header.destination_id != ingress->second.session->keys().local_id) {
    ++metrics_.ingress_rejected; return false;
  }
  NativeMessage message{}; message.header = opened.header; message.length = opened.length; std::copy(opened.payload.begin(), opened.payload.begin() + static_cast<std::ptrdiff_t>(opened.length), message.payload.begin());
  if (message.header.type == MessageType::Position) { Position decoded{}; if (!decode_position(message.payload.data(), message.length, decoded)) { ++metrics_.ingress_rejected; return false; } }
  if (state_type(message.header.type)) {
    const auto key = stream_key(message); const auto old = cache_.find(key);
    if (old != cache_.end()) {
      StateMeta old_meta{}; StateMeta new_meta{};
      if (old->second.length < 16U || message.length < 16U || !decode_state_meta(old->second.payload.data(), 16U, old_meta) || !decode_state_meta(message.payload.data(), 16U, new_meta)) { ++metrics_.ingress_rejected; return false; }
      if (message.header.type == MessageType::Position) { Position decoded{}; if (!decode_position(message.payload.data(), message.length, decoded)) { ++metrics_.ingress_rejected; return false; } }
      if (!new_meta.snapshot && new_meta.base_generation != old_meta.generation) { if (resync_.size() < queue_capacity_) { resync_.push_back(ResyncRequest{message.header.type, new_meta.source_id, new_meta.resource_id}); ++metrics_.resync_requests; } return false; }
      if (new_meta.generation <= old_meta.generation) return true;
    }
    cache_[key] = message; ++metrics_.cache_updates;
  }
  ++metrics_.ingress_accepted;
  for (auto& item : links_) {
    if (item.first == ingress_link) continue;
    const auto subscription = item.second.subscriptions.find(static_cast<std::uint8_t>(message.header.type));
    if (subscription == item.second.subscriptions.end() || subscription->second.source_id != message.header.source_id) continue;
    NativeMessage forwarded = message; forwarded.header.destination_id = item.first;
    if (item.second.queue.push(std::move(forwarded), now_ms)) ++metrics_.forwarded;
  }
  return true;
}

bool NativeProxy::next(std::uint16_t egress_link, NativeMessage& message, std::uint64_t now_ms) noexcept {
  const auto it = links_.find(egress_link); if (it == links_.end()) return false;
  return it->second.queue.pop(message, now_ms);
}

bool NativeProxy::cached(MessageType type, NativeMessage& message) const noexcept {
  for (const auto& item : cache_) if (item.second.header.type == type) { message = item.second; return true; }
  return false;
}

bool NativeProxy::next_resync(ResyncRequest& request) noexcept {
  if (resync_.empty()) return false;
  request = resync_.front(); resync_.pop_front(); return true;
}

} // namespace new_mavlink
