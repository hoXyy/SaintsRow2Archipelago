#pragma once

#include <optional>
#include <string_view>

#include "game/HudNotifications.hpp"
#include "sr2ap/src/ffi/session_ffi.rs.h"

namespace sr2ap {

// "Received X from Y", "Found your X" and "Sent X to Y", colored like the
// Archipelago text client. Empty for NotificationKind::None.
std::optional<NotificationText> FormatItemNotification(
    const rust::SessionNotification& notification);

// "(Deathlink) <cause>", the message the other player's game sent with the
// death. Falls back to "<source> died" when the game sent no message.
NotificationText FormatDeathLinkNotification(std::string_view source,
                                             std::string_view cause);

}  // namespace sr2ap
