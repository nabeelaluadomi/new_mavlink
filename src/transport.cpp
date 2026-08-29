#include "new_mavlink/transport.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace new_mavlink {

UdpEndpoint::~UdpEndpoint() { close(); }

bool UdpEndpoint::bind(std::uint16_t port) noexcept {
  close();
  fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
  if (fd_ < 0) return false;
  int reuse = 1;
  (void)::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  address.sin_port = htons(port);
  if (::bind(fd_, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) != 0) {
    close(); return false;
  }
  return true;
}

bool UdpEndpoint::send_to(const char* host, std::uint16_t port,
                          const std::uint8_t* bytes, std::size_t length) noexcept {
  if (fd_ < 0 || host == nullptr || bytes == nullptr || length == 0U || length > kMaxRecordBytes) return false;
  sockaddr_in address{};
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  if (::inet_pton(AF_INET, host, &address.sin_addr) != 1) return false;
  const ssize_t sent = ::sendto(fd_, bytes, length, 0, reinterpret_cast<const sockaddr*>(&address), sizeof(address));
  return sent == static_cast<ssize_t>(length);
}

bool UdpEndpoint::receive(std::uint8_t* bytes, std::size_t capacity,
                          std::size_t& length, std::uint32_t timeout_ms) noexcept {
  length = 0U;
  if (fd_ < 0 || bytes == nullptr || capacity < kMaxRecordBytes) return false;
  fd_set read_set;
  FD_ZERO(&read_set); FD_SET(fd_, &read_set);
  timeval timeout{};
  timeout.tv_sec = static_cast<time_t>(timeout_ms / 1000U);
  timeout.tv_usec = static_cast<suseconds_t>((timeout_ms % 1000U) * 1000U);
  const int ready = ::select(fd_ + 1, &read_set, nullptr, nullptr, &timeout);
  if (ready <= 0) return false;
  const ssize_t received = ::recvfrom(fd_, bytes, capacity, 0, nullptr, nullptr);
  if (received <= 0) return false;
  length = static_cast<std::size_t>(received);
  return true;
}

void UdpEndpoint::close() noexcept {
  if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
}

bool NativeEndpoint::send(const RecordHeader& header, const std::uint8_t* payload,
                          std::size_t length, UdpEndpoint& udp, const char* host,
                          std::uint16_t port, RecordError& error) noexcept {
  std::size_t wire_length = 0U;
  if (!session_.seal(header, payload, length, buffer_, wire_length, error)) return false;
  return udp.send_to(host, port, buffer_.data(), wire_length);
}

bool NativeEndpoint::receive_uncommitted(UdpEndpoint& udp, std::uint32_t now_ms,
                                          OpenRecord& record, RecordError& error) noexcept {
  std::size_t length = 0U;
  if (!udp.receive(buffer_.data(), buffer_.size(), length, 1000U)) {
    error = RecordError::Timeout; return false;
  }
  return session_.open_uncommitted(buffer_.data(), length, now_ms, record, error);
}

bool NativeEndpoint::commit(const OpenRecord& record, RecordError& error) noexcept {
  return session_.commit_replay(record, error);
}

bool NativeEndpoint::receive(UdpEndpoint& udp, std::uint32_t now_ms,
                             OpenRecord& record, RecordError& error) noexcept {
  if (!receive_uncommitted(udp, now_ms, record, error)) return false;
  return commit(record, error);
}

} // namespace new_mavlink
