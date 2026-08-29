#pragma once

#include "new_mavlink/record.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace new_mavlink {

class UdpEndpoint {
 public:
  UdpEndpoint() noexcept = default;
  ~UdpEndpoint();
  UdpEndpoint(const UdpEndpoint&) = delete;
  UdpEndpoint& operator=(const UdpEndpoint&) = delete;

  bool bind(std::uint16_t port) noexcept;
  bool send_to(const char* host, std::uint16_t port,
               const std::uint8_t* bytes, std::size_t length) noexcept;
  bool receive(std::uint8_t* bytes, std::size_t capacity,
               std::size_t& length, std::uint32_t timeout_ms) noexcept;
  void close() noexcept;
  bool valid() const noexcept { return fd_ >= 0; }

 private:
  int fd_{-1};
};

class NativeEndpoint {
 public:
  explicit NativeEndpoint(Session session) noexcept : session_(std::move(session)) {}

  bool send(const RecordHeader& header, const std::uint8_t* payload,
            std::size_t length, UdpEndpoint& udp, const char* host,
            std::uint16_t port, RecordError& error) noexcept;
  bool receive_uncommitted(UdpEndpoint& udp, std::uint32_t now_ms, OpenRecord& record,
                           RecordError& error) noexcept;
  bool commit(const OpenRecord& record, RecordError& error) noexcept;
  bool receive(UdpEndpoint& udp, std::uint32_t now_ms, OpenRecord& record,
               RecordError& error) noexcept;

 private:
  Session session_;
  std::array<std::uint8_t, kMaxRecordBytes> buffer_{};
};

} // namespace new_mavlink
