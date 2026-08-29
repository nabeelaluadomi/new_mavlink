#include "new_mavlink/record.hpp"
#include "new_mavlink/crypto.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>

int main() {
  using namespace new_mavlink;
  std::array<std::uint8_t, 16U> psk{}; for (std::size_t i = 0U; i < psk.size(); ++i) psk[i] = static_cast<std::uint8_t>(0xA0U + i);
  DirectionalKeys tx{}; DirectionalKeys rx{}; assert(derive_directional_keys(psk, SessionId{77U, 3U}, 1U, 2U, tx, rx));
  Session sender(tx); Session receiver(rx); RecordHeader header{}; header.type = MessageType::CommandRequest; header.source_id = 1U; header.destination_id = 2U; header.session_id = 77U; header.epoch = 3U; header.sequence = 4U;
  const std::array<std::uint8_t, 6U> plain{{'s','e','c','r','e','t'}}; std::array<std::uint8_t, kMaxRecordBytes> record{}; std::size_t record_length = 0U; RecordError error = RecordError::None; assert(sender.seal(header, plain.data(), plain.size(), record, record_length, error));
  std::size_t second_length = 0U; assert(!sender.seal(header, plain.data(), plain.size(), record, second_length, error));
  OpenRecord opened{}; if (!receiver.open_uncommitted(record.data(), record_length, 0U, opened, error)) { std::printf("OPEN_ERROR=%u\\n", static_cast<unsigned>(error)); return 10; } assert(opened.length == plain.size()); assert(receiver.commit_replay(opened, error));
  assert(!receiver.open_uncommitted(record.data(), record_length, 0U, opened, error)); assert(error == RecordError::Replay);
  record[record_length - 1U] ^= 1U; assert(!receiver.open_uncommitted(record.data(), record_length, 0U, opened, error)); assert(error == RecordError::AuthenticationFailed || error == RecordError::Replay);
  DirectionalKeys next_tx{}; DirectionalKeys next_rx{}; assert(derive_directional_keys(psk, SessionId{77U, 4U}, 1U, 2U, next_tx, next_rx)); Session next_sender(tx); assert(!next_sender.rekey(tx, error)); assert(next_sender.rekey(next_tx, error)); RecordHeader next_header = header; next_header.epoch = 4U; next_header.sequence = 0U; assert(next_sender.seal(next_header, plain.data(), plain.size(), record, record_length, error));
  return 0;
}
