#include "new_mavlink/record.hpp"

#include "new_mavlink/crc16.hpp"
#include "new_mavlink/crypto.hpp"

#include <algorithm>
#include <array>
#include <cstring>

namespace new_mavlink {
namespace {

void put16(std::uint8_t* p, std::uint16_t v) noexcept {
  p[0] = static_cast<std::uint8_t>(v & 0xFFU);
  p[1] = static_cast<std::uint8_t>(v >> 8U);
}
void put64(std::uint8_t* p, std::uint64_t v) noexcept {
  for (unsigned i = 0U; i < 8U; ++i) p[i] = static_cast<std::uint8_t>(v >> (8U * i));
}
std::uint16_t get16(const std::uint8_t* p) noexcept {
  return static_cast<std::uint16_t>(p[0]) | static_cast<std::uint16_t>(p[1] << 8U);
}
std::uint64_t get64(const std::uint8_t* p) noexcept {
  std::uint64_t v = 0U;
  for (unsigned i = 0U; i < 8U; ++i) v |= static_cast<std::uint64_t>(p[i]) << (8U * i);
  return v;
}

void encode_header(const RecordHeader& h, std::uint8_t* out, bool with_crc) noexcept {
  out[0] = h.version;
  out[1] = h.flags;
  out[2] = static_cast<std::uint8_t>(h.type);
  out[3] = static_cast<std::uint8_t>(h.qos);
  put16(out + 4U, h.source_id);
  put16(out + 6U, h.destination_id);
  put64(out + 8U, h.session_id);
  put64(out + 16U, h.epoch);
  put64(out + 24U, h.sequence);
  put16(out + 32U, h.payload_len);
  put16(out + 34U, h.deadline_ms);
  put16(out + 36U, with_crc ? h.header_crc16 : 0U);
  put16(out + 38U, 0U);
}

bool decode_header(const std::uint8_t* in, RecordHeader& h) noexcept {
  h = RecordHeader{};
  if (in == nullptr) return false;
  h.version = in[0]; h.flags = in[1]; h.type = static_cast<MessageType>(in[2]);
  h.qos = static_cast<Qos>(in[3]); h.source_id = get16(in + 4U);
  h.destination_id = get16(in + 6U); h.session_id = get64(in + 8U);
  h.epoch = get64(in + 16U); h.sequence = get64(in + 24U);
  h.payload_len = get16(in + 32U); h.deadline_ms = get16(in + 34U);
  h.header_crc16 = get16(in + 36U);
  if (in[38] != 0U || in[39] != 0U) return false;
  const std::uint16_t expected = crc16_ccitt(in, 36U);
  return expected == h.header_crc16;
}

bool valid_keys(const DirectionalKeys& k) noexcept {
  return k.session.value != 0U && k.session.epoch != 0U && k.local_id != 0U &&
         k.peer_id != 0U && k.local_id != k.peer_id &&
         std::any_of(k.tx.begin(), k.tx.end(), [](std::uint8_t x) { return x != 0U; }) &&
         std::any_of(k.rx.begin(), k.rx.end(), [](std::uint8_t x) { return x != 0U; }) &&
         k.tx != k.rx;
}

} // namespace

bool ReplayWindow::accepted(std::uint64_t sequence) const noexcept {
  if (!initialized_) return true;
  if (sequence > high_water_) return true;
  const std::uint64_t distance = high_water_ - sequence;
  return distance < 64U && (bitmap_ & (1ULL << distance)) == 0U;
}

bool ReplayWindow::commit(std::uint64_t sequence) noexcept {
  if (!accepted(sequence)) return false;
  if (!initialized_) {
    initialized_ = true; high_water_ = sequence; bitmap_ = 1ULL; return true;
  }
  if (sequence > high_water_) {
    const std::uint64_t shift = sequence - high_water_;
    bitmap_ = shift >= 64U ? 1ULL : ((bitmap_ << shift) | 1ULL);
    high_water_ = sequence;
  } else {
    bitmap_ |= 1ULL << (high_water_ - sequence);
  }
  return true;
}

void ReplayWindow::reset() noexcept {
  high_water_ = 0U; bitmap_ = 0U; initialized_ = false;
}

Session::Session(DirectionalKeys keys) noexcept : keys_(keys) {}

bool Session::seal(const RecordHeader& input,
                   const std::uint8_t* plaintext,
                   std::size_t length,
                   std::array<std::uint8_t, kMaxRecordBytes>& output,
                   std::size_t& output_length,
                   RecordError& error) noexcept {
  output_length = 0U; error = RecordError::None;
  if (!valid_keys(keys_)) { error = RecordError::InvalidKeys; return false; }
  if (input.type == MessageType::Hello || input.type == MessageType::Accept ||
      input.type == MessageType::Finish) { /* session control is still authenticated by session keys */ }
  if (!known_type(input.type) || input.flags > kKnownFlags || static_cast<std::uint8_t>(input.qos) > static_cast<std::uint8_t>(Qos::Explicit) || input.source_id != keys_.local_id ||
      input.destination_id != keys_.peer_id || input.session_id != keys_.session.value ||
      input.epoch != keys_.session.epoch || length > kMaxPayloadBytes ||
      (plaintext == nullptr && length != 0U) || input.sequence == UINT64_MAX ||
      (send_initialized_ && input.sequence <= send_high_water_)) {
    error = RecordError::InvalidHeader; return false;
  }
  RecordHeader h = input;
  h.version = kVersion;
  h.payload_len = static_cast<std::uint16_t>(length);
  h.header_crc16 = crc16_ccitt(nullptr, 0U);
  encode_header(h, output.data(), false);
  h.header_crc16 = crc16_ccitt(output.data(), 36U);
  encode_header(h, output.data(), true);
  std::array<std::uint8_t, kNonceBytes> nonce{};
  make_nonce(keys_.session, h.sequence, nonce);
  std::size_t cipher_length = 0U;
  if (!aead_seal(keys_.tx, nonce, output.data(), kHeaderBytes, plaintext, length,
                 output.data() + kHeaderBytes, output.size() - kHeaderBytes, cipher_length)) {
    error = RecordError::AuthenticationFailed; return false;
  }
  output_length = kHeaderBytes + cipher_length;
  send_high_water_ = h.sequence; send_initialized_ = true;
  return true;
}

bool Session::open_uncommitted(const std::uint8_t* record,
                               std::size_t record_length,
                               std::uint32_t now_ms,
                               OpenRecord& output,
                               RecordError& error) const noexcept {
  output = OpenRecord{}; error = RecordError::None;
  if (!valid_keys(keys_)) { error = RecordError::InvalidKeys; return false; }
  if (record == nullptr || record_length < kHeaderBytes + kTagBytes ||
      record_length > kMaxRecordBytes) { error = RecordError::InvalidLength; return false; }
  if (!decode_header(record, output.header) || output.header.version != kVersion ||
      !known_type(output.header.type) || output.header.flags > kKnownFlags ||
      static_cast<std::uint8_t>(output.header.qos) > static_cast<std::uint8_t>(Qos::Explicit) ||
      output.header.source_id != keys_.peer_id || output.header.destination_id != keys_.local_id ||
      output.header.session_id != keys_.session.value || output.header.epoch != keys_.session.epoch ||
      output.header.payload_len > kMaxPayloadBytes ||
      record_length != kHeaderBytes + output.header.payload_len + kTagBytes) {
    error = RecordError::InvalidHeader; return false;
  }
  if (output.header.deadline_ms != 0U && now_ms > output.header.deadline_ms) {
    error = RecordError::Expired; return false;
  }
  if (!replay_.accepted(output.header.sequence)) { error = RecordError::Replay; return false; }
  std::array<std::uint8_t, kNonceBytes> nonce{};
  make_nonce(keys_.session, output.header.sequence, nonce);
  if (!aead_open(keys_.rx, nonce, record, kHeaderBytes, record + kHeaderBytes,
                 record_length - kHeaderBytes, output.payload.data(), output.payload.size(),
                 output.length)) {
    error = RecordError::AuthenticationFailed; return false;
  }
  return true;
}

bool Session::commit_replay(const OpenRecord& output, RecordError& error) noexcept {
  error = RecordError::None;
  if (!replay_.commit(output.header.sequence)) { error = RecordError::Replay; return false; }
  return true;
}

bool Session::rekey(DirectionalKeys next_keys, RecordError& error) noexcept {
  error = RecordError::None;
  if (!valid_keys(next_keys) || next_keys.local_id != keys_.local_id || next_keys.peer_id != keys_.peer_id ||
      next_keys.session.value != keys_.session.value || next_keys.session.epoch <= keys_.session.epoch) {
    error = RecordError::InvalidHeader; return false;
  }
  keys_ = next_keys; replay_.reset(); send_high_water_ = 0U; send_initialized_ = false; return true;
}

} // namespace new_mavlink
