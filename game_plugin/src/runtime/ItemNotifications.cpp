#include "ItemNotifications.hpp"

#include <cstdint>
#include <string_view>

namespace sr2ap {
namespace {
// Archipelago item flags.
constexpr std::uint32_t kProgressionFlag = 0b001;
constexpr std::uint32_t kUsefulFlag = 0b010;
constexpr std::uint32_t kTrapFlag = 0b100;

// Archipelago text client colors.
constexpr std::uint32_t kProgressionColor = 0xAF99EF;
constexpr std::uint32_t kUsefulColor = 0x6D8BE8;
constexpr std::uint32_t kTrapColor = 0xFA8072;
constexpr std::uint32_t kFillerColor = 0x00EEEE;
constexpr std::uint32_t kOtherPlayerColor = 0xFAFAD2;

std::uint32_t ItemColor(const std::uint32_t flags) {
    if (flags & kProgressionFlag) {
        return kProgressionColor;
    }
    if (flags & kUsefulFlag) {
        return kUsefulColor;
    }
    if (flags & kTrapFlag) {
        return kTrapColor;
    }
    return kFillerColor;
}

std::string_view View(const ::rust::String& text) {
    return {text.data(), text.size()};
}
}  // namespace

std::optional<NotificationText> FormatItemNotification(
    const rust::SessionNotification& notification) {
    const auto item = View(notification.item);
    const auto player = View(notification.player);
    const auto itemColor = ItemColor(notification.flags);

    NotificationText text;
    switch (notification.kind) {
        case rust::NotificationKind::ReceivedItem:
            if (player.empty()) {
                text.Append("Found your ").Append(item, itemColor);
            } else {
                text.Append("Received ")
                    .Append(item, itemColor)
                    .Append(" from ")
                    .Append(player, kOtherPlayerColor);
            }
            return text;
        case rust::NotificationKind::SentItem:
            text.Append("Sent ")
                .Append(item, itemColor)
                .Append(" to ")
                .Append(player, kOtherPlayerColor);
            return text;
        default:
            return std::nullopt;
    }
}

}  // namespace sr2ap
