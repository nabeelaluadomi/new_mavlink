#include "new_mavlink/messages.hpp"
#include "new_mavlink/record.hpp"

#include <array>
#include <cassert>
#include <cstdint>

int main() {
  using namespace new_mavlink;
  std::uint32_t x = 0xC001D00DU;
  std::array<std::uint8_t, kMaxRecordBytes> bytes{};
  for (std::size_t i = 0U; i < 100000U; ++i) {
    for (auto& byte : bytes) { x = x * 1664525U + 1013904223U; byte = static_cast<std::uint8_t>(x >> 24U); }
    CommandRequest command{}; (void)decode_command(bytes.data(), i % (kMaxRecordBytes + 1U), command);
    Subscription subscription{}; (void)decode_subscription(bytes.data(), i % (kMaxRecordBytes + 1U), subscription);
    ResyncRequest resync{}; (void)decode_resync(bytes.data(), i % (kMaxRecordBytes + 1U), resync);
    DirectionalKeys keys{}; Session session(keys); OpenRecord opened{}; RecordError error = RecordError::None; (void)session.open_uncommitted(bytes.data(), bytes.size(), 0U, opened, error);
  }
  return 0;
}
