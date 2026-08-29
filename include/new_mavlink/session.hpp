#pragma once

#include "new_mavlink/crypto.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace new_mavlink {

enum class SessionError : std::uint8_t {
  None = 0U,
  InvalidInput,
  InvalidState,
  AuthenticationFailed,
  NonceReuse,
  InvalidTranscript,
};

struct Hello {
  std::uint16_t initiator_id{0U};
  std::uint16_t responder_id{0U};
  std::uint64_t client_nonce{0U};
  std::uint32_t capabilities{0U};
};

struct Accept {
  std::uint16_t initiator_id{0U};
  std::uint16_t responder_id{0U};
  std::uint64_t client_nonce{0U};
  std::uint64_t server_nonce{0U};
  std::uint64_t session_id{0U};
  std::uint64_t epoch{0U};
  std::array<std::uint8_t, 16U> authenticator{};
};

struct Finish {
  std::uint16_t initiator_id{0U};
  std::uint16_t responder_id{0U};
  std::uint64_t session_id{0U};
  std::uint64_t epoch{0U};
  std::array<std::uint8_t, 16U> authenticator{};
};

constexpr std::size_t kHelloBytes = 24U;
constexpr std::size_t kAcceptBytes = 64U;
constexpr std::size_t kFinishBytes = 48U;

bool encode_hello(const Hello&, std::array<std::uint8_t, kHelloBytes>&) noexcept;
bool decode_hello(const std::uint8_t*, std::size_t, Hello&) noexcept;
bool encode_accept(const Accept&, std::array<std::uint8_t, kAcceptBytes>&) noexcept;
bool decode_accept(const std::uint8_t*, std::size_t, Accept&) noexcept;
bool encode_finish(const Finish&, std::array<std::uint8_t, kFinishBytes>&) noexcept;
bool decode_finish(const std::uint8_t*, std::size_t, Finish&) noexcept;

class InitiatorSession {
 public:
  InitiatorSession(std::uint16_t local_id,
                   std::uint16_t peer_id,
                   std::array<std::uint8_t, 16U> psk) noexcept;
  bool start(std::uint64_t client_nonce, Hello& hello, SessionError& error) noexcept;
  bool accept(const Accept& accept, Finish& finish, SessionError& error) noexcept;
  bool established() const noexcept { return established_; }
  const DirectionalKeys& keys() const noexcept { return keys_; }

 private:
  std::uint16_t local_id_{0U};
  std::uint16_t peer_id_{0U};
  std::array<std::uint8_t, 16U> psk_{};
  Hello hello_{};
  DirectionalKeys keys_{};
  bool started_{false};
  bool established_{false};
};

class ResponderSession {
 public:
  ResponderSession(std::uint16_t local_id,
                   std::uint16_t peer_id,
                   std::array<std::uint8_t, 16U> psk) noexcept;
  bool receive_hello(const Hello&, std::uint64_t server_nonce,
                     Accept& accept, SessionError& error) noexcept;
  bool finish(const Finish&, SessionError& error) noexcept;
  bool established() const noexcept { return established_; }
  const DirectionalKeys& keys() const noexcept { return keys_; }

 private:
  std::uint16_t local_id_{0U};
  std::uint16_t peer_id_{0U};
  std::array<std::uint8_t, 16U> psk_{};
  Hello hello_{};
  Accept accept_{};
  DirectionalKeys keys_{};
  bool accepted_{false};
  bool established_{false};
};

} // namespace new_mavlink
