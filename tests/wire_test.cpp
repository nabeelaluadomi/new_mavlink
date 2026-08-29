#include "new_mavlink/wire.hpp"

#include <cassert>

int main() {
  using namespace new_mavlink;
  assert(known_type(MessageType::Position));
  assert(state_type(MessageType::Battery));
  assert(default_qos(MessageType::Position) == Qos::StateLatest);
  assert(default_qos(MessageType::CommandRequest) == Qos::CommandReliable);
  assert(!known_type(static_cast<MessageType>(255U)));
  return 0;
}
