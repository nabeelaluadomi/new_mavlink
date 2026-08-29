#pragma once

#include "new_mavlink/wire.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace new_mavlink {

bool derive_directional_keys(const std::array<std::uint8_t, 16U>& psk,
                             const SessionId& session,
                             std::uint16_t first_id,
                             std::uint16_t second_id,
                             DirectionalKeys& first,
                             DirectionalKeys& second) noexcept;

bool aead_seal(const std::array<std::uint8_t, kKeyBytes>& key,
               const std::array<std::uint8_t, kNonceBytes>& nonce,
               const std::uint8_t* aad,
               std::size_t aad_length,
               const std::uint8_t* plaintext,
               std::size_t plaintext_length,
               std::uint8_t* ciphertext,
               std::size_t ciphertext_capacity,
               std::size_t& ciphertext_length) noexcept;

bool aead_open(const std::array<std::uint8_t, kKeyBytes>& key,
               const std::array<std::uint8_t, kNonceBytes>& nonce,
               const std::uint8_t* aad,
               std::size_t aad_length,
               const std::uint8_t* ciphertext,
               std::size_t ciphertext_length,
               std::uint8_t* plaintext,
               std::size_t plaintext_capacity,
               std::size_t& plaintext_length) noexcept;

void make_nonce(const SessionId& session,
                std::uint64_t sequence,
                std::array<std::uint8_t, kNonceBytes>& nonce) noexcept;

} // namespace new_mavlink
