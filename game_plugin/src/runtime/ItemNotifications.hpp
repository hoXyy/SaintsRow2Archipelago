#pragma once

#include <optional>

#include "game/HudNotifications.hpp"
#include "sr2ap/src/ffi/session_ffi.rs.h"

namespace sr2ap {

// "Received X from Y", "Found your X" and "Sent X to Y", colored like the
// Archipelago text client. Empty for NotificationKind::None.
std::optional<NotificationText> FormatItemNotification(
    const rust::SessionNotification& notification);

}  // namespace sr2ap
