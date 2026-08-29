#include "new_mavlink/wire.hpp"

namespace new_mavlink {

bool known_type(MessageType type) noexcept {
  switch (type) {
    case MessageType::Hello:
    case MessageType::Accept:
    case MessageType::Finish:
    case MessageType::Subscribe:
    case MessageType::Resync:
    case MessageType::Unsubscribe:
    case MessageType::Heartbeat:
    case MessageType::VehicleState:
    case MessageType::Position:
    case MessageType::Attitude:
    case MessageType::Battery:
    case MessageType::LinkStats:
    case MessageType::SafetyEvent:
    case MessageType::FailsafeEvent:
    case MessageType::GeofenceEvent:
    case MessageType::CommandRequest:
    case MessageType::CommandAck:
    case MessageType::CommandResult:
    case MessageType::SchemaManifest:
    case MessageType::RateReport:
    case MessageType::DiagnosticEvent:
      return true;
  }
  return false;
}

bool state_type(MessageType type) noexcept {
  return type == MessageType::VehicleState || type == MessageType::Position ||
         type == MessageType::Attitude || type == MessageType::Battery ||
         type == MessageType::LinkStats || type == MessageType::RateReport;
}

bool command_type(MessageType type) noexcept {
  return type == MessageType::CommandRequest || type == MessageType::CommandAck ||
         type == MessageType::CommandResult;
}

bool control_type(MessageType type) noexcept {
  return type == MessageType::Hello || type == MessageType::Accept ||
         type ==          MessageType::Finish || type == MessageType::Subscribe ||
         type == MessageType::Unsubscribe || type == MessageType::Resync;
}

bool critical_type(MessageType type) noexcept {
  return type == MessageType::SafetyEvent || type == MessageType::FailsafeEvent ||
         type == MessageType::GeofenceEvent || type == MessageType::DiagnosticEvent || type == MessageType::CommandAck ||
         type == MessageType::CommandResult;
}

Qos default_qos(MessageType type) noexcept {
  if (command_type(type)) {
    return Qos::CommandReliable;
  }
  if (critical_type(type)) {
    return Qos::CriticalReliable;
  }
  if (type == MessageType::Heartbeat) {
    return Qos::Liveness;
  }
  if (control_type(type)) {
    return Qos::Explicit;
  }
  return Qos::StateLatest;
}

} // namespace new_mavlink
