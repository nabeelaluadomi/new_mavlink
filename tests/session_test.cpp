#include "new_mavlink/session.hpp"
#include "new_mavlink/record.hpp"

#include <array>
#include <cassert>

int main() {
  using namespace new_mavlink;
  std::array<std::uint8_t, 16U> psk{}; for (std::size_t i = 0U; i < psk.size(); ++i) psk[i] = static_cast<std::uint8_t>(i + 1U);
  InitiatorSession initiator(10U, 20U, psk); ResponderSession responder(20U, 10U, psk);
  Hello hello{}; SessionError error = SessionError::None; assert(initiator.start(0x1122334455667788ULL, hello, error));
  Accept accept{}; assert(responder.receive_hello(hello, 0x8877665544332211ULL, accept, error));
  Finish finish{}; assert(initiator.accept(accept, finish, error)); assert(responder.finish(finish, error)); assert(initiator.established() && responder.established());
  RecordHeader header{}; header.type = MessageType::Heartbeat; header.source_id = 10U; header.destination_id = 20U; header.session_id = initiator.keys().session.value; header.epoch = initiator.keys().session.epoch; header.sequence = 0U;
  std::array<std::uint8_t, kMaxRecordBytes> wire{}; std::size_t wire_length = 0U; RecordError record_error = RecordError::None; const std::array<std::uint8_t, 1U> payload{{0x42U}}; assert(Session(initiator.keys()).seal(header, payload.data(), payload.size(), wire, wire_length, record_error));
  OpenRecord opened{}; Session responder_record(responder.keys()); assert(responder_record.open_uncommitted(wire.data(), wire_length, 0U, opened, record_error)); assert(opened.length == 1U && opened.payload[0] == 0x42U); assert(responder_record.commit_replay(opened, record_error));
  return 0;
}
