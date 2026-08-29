#pragma once

#include "new_mavlink/wire.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace new_mavlink {

enum class RecordError : std::uint8_t {
  None = 0U,
  InvalidHeader,
  InvalidKeys,
  InvalidLength,
  AuthenticationFailed,
  Replay,
  Expired,
  SequenceExhausted,
  InvalidPayload,
  Timeout,
};

struct OpenRecord {
  RecordHeader header{};
  std::array<std::uint8_t, kMaxPayloadBytes> payload{};
  std::size_t length{0U};
};

class ReplayWindow {
 public:
  bool accepted(std::uint64_t sequence) const noexcept;
  bool commit(std::uint64_t sequence) noexcept;
  void reset() noexcept;

 private:
  std::uint64_t high_water_{0U};
  std::uint64_t bitmap_{0U};
  bool initialized_{false};
};

class Session {
 public:
  explicit Session(DirectionalKeys keys) noexcept;

  bool seal(const RecordHeader& header,
            const std::uint8_t* plaintext,
            std::size_t length,
            std::array<std::uint8_t, kMaxRecordBytes>& output,
            std::size_t& output_length,
            RecordError& error) noexcept;

  bool open_uncommitted(const std::uint8_t* record,
                        std::size_t record_length,
                        std::uint32_t now_ms,
                        OpenRecord& output,
                        RecordError& error) const noexcept;

  bool commit_replay(const OpenRecord& output, RecordError& error) noexcept;
  bool rekey(DirectionalKeys next_keys, RecordError& error) noexcept;
  const DirectionalKeys& keys() const noexcept { return keys_; }

 private:
  DirectionalKeys keys_{};
  ReplayWindow replay_{};
  std::uint64_t send_high_water_{0U};
  bool send_initialized_{false};
};

} // namespace new_mavlink
