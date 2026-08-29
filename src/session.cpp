#include "new_mavlink/session.hpp"

#include "new_mavlink/crc16.hpp"

#include <array>
#include <cstring>

extern "C" {
#include "crypto_aead.h"
}

namespace new_mavlink {
namespace {

void put16(std::uint8_t* p, std::uint16_t v) noexcept { p[0] = static_cast<std::uint8_t>(v); p[1] = static_cast<std::uint8_t>(v >> 8U); }
void put32(std::uint8_t* p, std::uint32_t v) noexcept { for (unsigned i = 0U; i < 4U; ++i) p[i] = static_cast<std::uint8_t>(v >> (8U * i)); }
void put64(std::uint8_t* p, std::uint64_t v) noexcept { for (unsigned i = 0U; i < 8U; ++i) p[i] = static_cast<std::uint8_t>(v >> (8U * i)); }
std::uint16_t get16(const std::uint8_t* p) noexcept { return static_cast<std::uint16_t>(p[0]) | static_cast<std::uint16_t>(p[1] << 8U); }
std::uint32_t get32(const std::uint8_t* p) noexcept { std::uint32_t v = 0U; for (unsigned i = 0U; i < 4U; ++i) v |= static_cast<std::uint32_t>(p[i]) << (8U * i); return v; }
std::uint64_t get64(const std::uint8_t* p) noexcept { std::uint64_t v = 0U; for (unsigned i = 0U; i < 8U; ++i) v |= static_cast<std::uint64_t>(p[i]) << (8U * i); return v; }

bool valid_ids(std::uint16_t a, std::uint16_t b) noexcept { return a != 0U && b != 0U && a != b; }

void transcript(const Hello& hello, const Accept& accept, std::uint8_t* out, std::size_t& length) noexcept {
  std::array<std::uint8_t, 52U> bytes{};
  put16(bytes.data(), hello.initiator_id); put16(bytes.data() + 2U, hello.responder_id); put64(bytes.data() + 4U, hello.client_nonce); put32(bytes.data() + 12U, hello.capabilities);
  put16(bytes.data() + 16U, accept.initiator_id); put16(bytes.data() + 18U, accept.responder_id); put64(bytes.data() + 20U, accept.client_nonce); put64(bytes.data() + 28U, accept.server_nonce); put64(bytes.data() + 36U, accept.session_id); put64(bytes.data() + 44U, accept.epoch);
  std::memcpy(out, bytes.data(), bytes.size()); length = bytes.size();
}

bool make_auth(const std::array<std::uint8_t, 16U>& psk, const std::uint8_t* input, std::size_t input_length, std::array<std::uint8_t, 16U>& auth) noexcept {
  std::array<std::uint8_t, 16U> nonce{};
  std::array<std::uint8_t, 16U> ciphertext{};
  unsigned long long out_len = 0ULL;
  const int rc = crypto_aead_encrypt(ciphertext.data(), &out_len, nullptr, 0ULL, input, static_cast<unsigned long long>(input_length), nullptr, nonce.data(), psk.data());
  if (rc != 0 || out_len != ciphertext.size()) return false;
  std::copy(ciphertext.begin(), ciphertext.end(), auth.begin());
  return true;
}

} // namespace

bool encode_hello(const Hello& v, std::array<std::uint8_t, kHelloBytes>& out) noexcept {
  out.fill(0U); if (!valid_ids(v.initiator_id, v.responder_id) || v.client_nonce == 0U) return false;
  put16(out.data(), v.initiator_id); put16(out.data() + 2U, v.responder_id); put64(out.data() + 4U, v.client_nonce); put32(out.data() + 12U, v.capabilities); return crc16_ccitt(out.data(), 16U) != 0U;
}

bool decode_hello(const std::uint8_t* bytes, std::size_t length, Hello& v) noexcept {
  v = Hello{}; if (bytes == nullptr || length != kHelloBytes || get16(bytes + 16U) != crc16_ccitt(bytes, 16U)) return false;
  v.initiator_id = get16(bytes); v.responder_id = get16(bytes + 2U); v.client_nonce = get64(bytes + 4U); v.capabilities = get32(bytes + 12U); return valid_ids(v.initiator_id, v.responder_id) && v.client_nonce != 0U;
}

bool encode_accept(const Accept& v, std::array<std::uint8_t, kAcceptBytes>& out) noexcept {
  out.fill(0U); if (!valid_ids(v.initiator_id, v.responder_id) || v.client_nonce == 0U || v.server_nonce == 0U || v.session_id == 0U || v.epoch == 0U) return false;
  put16(out.data(), v.initiator_id); put16(out.data() + 2U, v.responder_id); put64(out.data() + 4U, v.client_nonce); put64(out.data() + 12U, v.server_nonce); put64(out.data() + 20U, v.session_id); put64(out.data() + 28U, v.epoch); std::copy(v.authenticator.begin(), v.authenticator.end(), out.begin() + 36); put16(out.data() + 52U, crc16_ccitt(out.data(), 52U)); return true;
}

bool decode_accept(const std::uint8_t* bytes, std::size_t length, Accept& v) noexcept {
  v = Accept{}; if (bytes == nullptr || length != kAcceptBytes || get16(bytes + 52U) != crc16_ccitt(bytes, 52U)) return false;
  v.initiator_id = get16(bytes); v.responder_id = get16(bytes + 2U); v.client_nonce = get64(bytes + 4U); v.server_nonce = get64(bytes + 12U); v.session_id = get64(bytes + 20U); v.epoch = get64(bytes + 28U); std::copy(bytes + 36U, bytes + 52U, v.authenticator.begin()); return valid_ids(v.initiator_id, v.responder_id) && v.client_nonce != 0U && v.server_nonce != 0U && v.session_id != 0U && v.epoch != 0U;
}

bool encode_finish(const Finish& v, std::array<std::uint8_t, kFinishBytes>& out) noexcept {
  out.fill(0U); if (!valid_ids(v.initiator_id, v.responder_id) || v.session_id == 0U || v.epoch == 0U) return false;
  put16(out.data(), v.initiator_id); put16(out.data() + 2U, v.responder_id); put64(out.data() + 4U, v.session_id); put64(out.data() + 12U, v.epoch); std::copy(v.authenticator.begin(), v.authenticator.end(), out.begin() + 20); put16(out.data() + 36U, crc16_ccitt(out.data(), 36U)); return true;
}

bool decode_finish(const std::uint8_t* bytes, std::size_t length, Finish& v) noexcept {
  v = Finish{}; if (bytes == nullptr || length != kFinishBytes || get16(bytes + 36U) != crc16_ccitt(bytes, 36U)) return false;
  v.initiator_id = get16(bytes); v.responder_id = get16(bytes + 2U); v.session_id = get64(bytes + 4U); v.epoch = get64(bytes + 12U); std::copy(bytes + 20U, bytes + 36U, v.authenticator.begin()); return valid_ids(v.initiator_id, v.responder_id) && v.session_id != 0U && v.epoch != 0U;
}

InitiatorSession::InitiatorSession(std::uint16_t local_id, std::uint16_t peer_id, std::array<std::uint8_t, 16U> psk) noexcept : local_id_(local_id), peer_id_(peer_id), psk_(psk) {}

bool InitiatorSession::start(std::uint64_t client_nonce, Hello& hello, SessionError& error) noexcept {
  error = SessionError::None; if (started_ || !valid_ids(local_id_, peer_id_) || client_nonce == 0U) { error = started_ ? SessionError::NonceReuse : SessionError::InvalidInput; return false; }
  hello_ = Hello{local_id_, peer_id_, client_nonce, 0U}; hello = hello_; started_ = true; return true;
}

bool InitiatorSession::accept(const Accept& received, Finish& finish_message, SessionError& error) noexcept {
  error = SessionError::None; if (!started_ || established_) { error = SessionError::InvalidState; return false; }
  if (received.initiator_id != local_id_ || received.responder_id != peer_id_ || received.client_nonce != hello_.client_nonce) { error = SessionError::InvalidTranscript; return false; }
  std::array<std::uint8_t, 52U> tr{}; std::size_t tr_len = 0U; transcript(hello_, received, tr.data(), tr_len); std::array<std::uint8_t, 16U> expected{};
  Accept without_auth = received; without_auth.authenticator.fill(0U); transcript(hello_, without_auth, tr.data(), tr_len); if (!make_auth(psk_, tr.data(), tr_len, expected) || expected != received.authenticator) { error = SessionError::AuthenticationFailed; return false; }
  SessionId sid{received.session_id, received.epoch}; DirectionalKeys initiator_keys{}; DirectionalKeys responder_keys{}; if (!derive_directional_keys(psk_, sid, local_id_, peer_id_, initiator_keys, responder_keys)) { error = SessionError::InvalidInput; return false; } keys_ = initiator_keys;
  finish_message = Finish{local_id_, peer_id_, received.session_id, received.epoch, {}}; std::array<std::uint8_t, 48U> f{}; put16(f.data(), finish_message.initiator_id); put16(f.data() + 2U, finish_message.responder_id); put64(f.data() + 4U, finish_message.session_id); put64(f.data() + 12U, finish_message.epoch); if (!make_auth(psk_, f.data(), 20U, finish_message.authenticator)) { error = SessionError::AuthenticationFailed; return false; } established_ = true; return true;
}

ResponderSession::ResponderSession(std::uint16_t local_id, std::uint16_t peer_id, std::array<std::uint8_t, 16U> psk) noexcept : local_id_(local_id), peer_id_(peer_id), psk_(psk) {}

bool ResponderSession::receive_hello(const Hello& hello, std::uint64_t server_nonce, Accept& response, SessionError& error) noexcept {
  error = SessionError::None; if (accepted_ || !valid_ids(local_id_, peer_id_) || hello.initiator_id != peer_id_ || hello.responder_id != local_id_ || hello.client_nonce == 0U || server_nonce == 0U) { error = accepted_ ? SessionError::NonceReuse : SessionError::InvalidInput; return false; }
  hello_ = hello; accept_ = Accept{peer_id_, local_id_, hello.client_nonce, server_nonce, hello.client_nonce ^ server_nonce, 1U, {}}; std::array<std::uint8_t, 52U> tr{}; std::size_t tr_len = 0U; Accept no_auth = accept_; transcript(hello_, no_auth, tr.data(), tr_len); if (!make_auth(psk_, tr.data(), tr_len, accept_.authenticator)) { error = SessionError::AuthenticationFailed; return false; } response = accept_; SessionId sid{accept_.session_id, accept_.epoch}; DirectionalKeys initiator_keys{}; if (!derive_directional_keys(psk_, sid, peer_id_, local_id_, initiator_keys, keys_)) { error = SessionError::InvalidInput; return false; } accepted_ = true; return true;
}

bool ResponderSession::finish(const Finish& message, SessionError& error) noexcept {
  error = SessionError::None; if (!accepted_ || established_ || message.initiator_id != peer_id_ || message.responder_id != local_id_ || message.session_id != accept_.session_id || message.epoch != accept_.epoch) { error = SessionError::InvalidState; return false; }
  std::array<std::uint8_t, 20U> input{}; put16(input.data(), message.initiator_id); put16(input.data() + 2U, message.responder_id); put64(input.data() + 4U, message.session_id); put64(input.data() + 12U, message.epoch); std::array<std::uint8_t, 16U> expected{}; if (!make_auth(psk_, input.data(), input.size(), expected) || expected != message.authenticator) { error = SessionError::AuthenticationFailed; return false; } established_ = true; return true;
}

} // namespace new_mavlink
