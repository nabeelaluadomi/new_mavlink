#include "new_mavlink/crypto.hpp"

#include <algorithm>
#include <array>
#include <cstring>

extern "C" {
#include "crypto_aead.h"
#include "crypto_auth.h"
}

namespace new_mavlink {
namespace {

void put16(std::uint8_t* p, std::uint16_t v) noexcept {
  p[0] = static_cast<std::uint8_t>(v & 0xFFU);
  p[1] = static_cast<std::uint8_t>(v >> 8U);
}

void put64(std::uint8_t* p, std::uint64_t v) noexcept {
  for (unsigned i = 0U; i < 8U; ++i) {
    p[i] = static_cast<std::uint8_t>(v >> (8U * i));
  }
}

void wipe(void* ptr, std::size_t length) noexcept {
  volatile std::uint8_t* p = static_cast<volatile std::uint8_t*>(ptr);
  for (std::size_t i = 0U; i < length; ++i) {
    p[i] = 0U;
  }
}

bool prf_block(const std::array<std::uint8_t, 16U>& psk,
               const std::uint8_t* info,
               std::size_t info_length,
               std::uint32_t block,
               std::array<std::uint8_t, 16U>& output) noexcept {
  if (info == nullptr || info_length > 96U) {
    return false;
  }
  std::array<std::uint8_t, 100U> input{};
  if (info_length != 0U) {
    std::memcpy(input.data(), info, info_length);
  }
  input[info_length + 0U] = static_cast<std::uint8_t>(block);
  input[info_length + 1U] = static_cast<std::uint8_t>(block >> 8U);
  input[info_length + 2U] = static_cast<std::uint8_t>(block >> 16U);
  input[info_length + 3U] = static_cast<std::uint8_t>(block >> 24U);
  const int rc = crypto_auth(output.data(), input.data(), info_length + 4U,
                             psk.data());
  wipe(input.data(), input.size());
  return rc == 0;
}

bool derive_one(const std::array<std::uint8_t, 16U>& psk,
                const std::uint8_t* label,
                std::size_t label_length,
                const SessionId& session,
                std::uint16_t first_id,
                std::uint16_t second_id,
                std::array<std::uint8_t, 16U>& key) noexcept {
  std::array<std::uint8_t, 32U> info{};
  if (label_length > 8U) {
    return false;
  }
  std::memcpy(info.data(), label, label_length);
  put64(info.data() + 8U, session.value);
  put64(info.data() + 16U, session.epoch);
  put16(info.data() + 24U, first_id);
  put16(info.data() + 26U, second_id);
  return prf_block(psk, info.data(), 28U, 0U, key);
}

} // namespace

void make_nonce(const SessionId& session,
                std::uint64_t sequence,
                std::array<std::uint8_t, kNonceBytes>& nonce) noexcept {
  put64(nonce.data(), session.value);
  put64(nonce.data() + 8U, session.epoch ^ sequence);
}

bool derive_directional_keys(const std::array<std::uint8_t, 16U>& psk,
                             const SessionId& session,
                             std::uint16_t first_id,
                             std::uint16_t second_id,
                             DirectionalKeys& first,
                             DirectionalKeys& second) noexcept {
  if (session.value == 0U || session.epoch == 0U || first_id == 0U ||
      second_id == 0U || first_id == second_id) {
    return false;
  }
  static constexpr std::uint8_t tx_label[] = {'N','M','-','T','X'};
  static constexpr std::uint8_t rx_label[] = {'N','M','-','R','X'};
  std::array<std::uint8_t, 16U> first_tx{};
  std::array<std::uint8_t, 16U> second_tx{};
  if (!derive_one(psk, tx_label, sizeof(tx_label), session, first_id,
                  second_id, first_tx) ||
      !derive_one(psk, rx_label, sizeof(rx_label), session, first_id,
                  second_id, second_tx)) {
    return false;
  }
  first = DirectionalKeys{session, first_id, second_id, first_tx, second_tx};
  second = DirectionalKeys{session, second_id, first_id, second_tx, first_tx};
  wipe(first_tx.data(), first_tx.size());
  wipe(second_tx.data(), second_tx.size());
  return true;
}

bool aead_seal(const std::array<std::uint8_t, kKeyBytes>& key,
               const std::array<std::uint8_t, kNonceBytes>& nonce,
               const std::uint8_t* aad,
               std::size_t aad_length,
               const std::uint8_t* plaintext,
               std::size_t plaintext_length,
               std::uint8_t* ciphertext,
               std::size_t ciphertext_capacity,
               std::size_t& ciphertext_length) noexcept {
  ciphertext_length = 0U;
  if (ciphertext == nullptr || ciphertext_capacity < plaintext_length + kTagBytes ||
      (aad == nullptr && aad_length != 0U) ||
      (plaintext == nullptr && plaintext_length != 0U) ||
      plaintext_length > kMaxPayloadBytes) {
    return false;
  }
  unsigned long long out_length = 0ULL;
  const int rc = crypto_aead_encrypt(ciphertext, &out_length, plaintext,
                                      static_cast<unsigned long long>(plaintext_length),
                                      aad, static_cast<unsigned long long>(aad_length),
                                      nullptr, nonce.data(), key.data());
  if (rc != 0 || out_length != plaintext_length + kTagBytes) {
    return false;
  }
  ciphertext_length = static_cast<std::size_t>(out_length);
  return true;
}

bool aead_open(const std::array<std::uint8_t, kKeyBytes>& key,
               const std::array<std::uint8_t, kNonceBytes>& nonce,
               const std::uint8_t* aad,
               std::size_t aad_length,
               const std::uint8_t* ciphertext,
               std::size_t ciphertext_length,
               std::uint8_t* plaintext,
               std::size_t plaintext_capacity,
               std::size_t& plaintext_length) noexcept {
  plaintext_length = 0U;
  if (ciphertext == nullptr || ciphertext_length < kTagBytes ||
      plaintext == nullptr || plaintext_capacity < ciphertext_length - kTagBytes ||
      ciphertext_length > kMaxPayloadBytes + kTagBytes ||
      (aad == nullptr && aad_length != 0U)) {
    return false;
  }
  unsigned long long out_length = 0ULL;
  const int rc = crypto_aead_decrypt(plaintext, &out_length, nullptr, ciphertext,
                                     static_cast<unsigned long long>(ciphertext_length),
                                     aad, static_cast<unsigned long long>(aad_length),
                                     nonce.data(), key.data());
  if (rc != 0 || out_length != ciphertext_length - kTagBytes) {
    return false;
  }
  plaintext_length = static_cast<std::size_t>(out_length);
  return true;
}

} // namespace new_mavlink
