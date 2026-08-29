#include "new_mavlink/mavlink2_adapter.hpp"

extern "C" {
#include "mavlink.h"
}

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char** argv) {
  using namespace new_mavlink;
  using namespace new_mavlink::mavlink2_adapter;
  const std::uint16_t local_port = argc > 1 ? static_cast<std::uint16_t>(std::strtoul(argv[1], nullptr, 10)) : 14540U;
  const std::uint16_t px4_port = argc > 2 ? static_cast<std::uint16_t>(std::strtoul(argv[2], nullptr, 10)) : 14580U;
  const std::uint16_t command = argc > 3 ? static_cast<std::uint16_t>(std::strtoul(argv[3], nullptr, 10)) : 400U;
  if (!command_allowed(command)) { std::printf("NEWMAVLINK_PX4_ADAPTER_REJECT=PASS command=%u\n", command); return 0; }
  const int fd = ::socket(AF_INET, SOCK_DGRAM, 0); if (fd < 0) return 2;
  sockaddr_in local{}; local.sin_family = AF_INET; local.sin_addr.s_addr = htonl(INADDR_LOOPBACK); local.sin_port = htons(local_port);
  if (::bind(fd, reinterpret_cast<const sockaddr*>(&local), sizeof(local)) != 0) { ::close(fd); return 3; }
  sockaddr_in remote{}; remote.sin_family = AF_INET; remote.sin_addr.s_addr = htonl(INADDR_LOOPBACK); remote.sin_port = htons(px4_port);
  mavlink_message_t heartbeat{}; mavlink_msg_heartbeat_pack(255U, 190U, &heartbeat, MAV_TYPE_GCS, MAV_AUTOPILOT_INVALID, 0U, 0U, MAV_STATE_ACTIVE);
  std::uint8_t buffer[280U]{}; std::uint16_t length = mavlink_msg_to_send_buffer(buffer, &heartbeat); (void)::sendto(fd, buffer, length, 0, reinterpret_cast<const sockaddr*>(&remote), sizeof(remote));
  CommandRequest request{}; request.request_id = 1U; request.command_id = command; request.deadline_ms = 3000U; request.arg_count = 1U; request.args[0] = command == 400U ? 1.0F : 0.0F;
  MavlinkFrame frame{}; if (!encode_command_long(request, 255U, 190U, 1U, 1U, frame)) { ::close(fd); return 4; }
  if (::sendto(fd, frame.bytes.data(), frame.length, 0, reinterpret_cast<const sockaddr*>(&remote), sizeof(remote)) != static_cast<ssize_t>(frame.length)) { ::close(fd); return 5; }
  const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3); CommandAck ack{}; bool found = false;
  while (std::chrono::steady_clock::now() < deadline) {
    const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
    fd_set set; FD_ZERO(&set); FD_SET(fd, &set); timeval timeout{static_cast<time_t>(remaining / 1000), static_cast<suseconds_t>((remaining % 1000) * 1000)};
    const int ready = ::select(fd + 1, &set, nullptr, nullptr, &timeout); if (ready <= 0) break;
    const ssize_t received = ::recvfrom(fd, buffer, sizeof(buffer), 0, nullptr, nullptr); if (received <= 0) break;
    if (decode_command_ack(buffer, static_cast<std::size_t>(received), ack) && ack.result_code == command) { found = true; break; }
  }
  ::close(fd); if (!found) { std::printf("NEWMAVLINK_PX4_ADAPTER_ACK=TIMEOUT_OR_INVALID command=%u\n", command); return 6; }
  std::printf("NEWMAVLINK_PX4_ADAPTER_ACK=PASS command=%u status=%u\n", command, ack.status); return 0;
}
