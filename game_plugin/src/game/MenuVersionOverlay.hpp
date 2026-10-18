#pragma once

#include <atomic>
#include <cstdint>
#include <optional>
#include <string>

#include "TextRenderer.hpp"

namespace sr2ap {

enum class ConnectionStatus : std::uint8_t {
    Disabled,
    Disconnected,
    WaitingForSession,
    Active,
};

// Draws the plugin version and AP client connection status on the main menu
// and the pause menu.
class MenuVersionOverlay {
   public:
    explicit MenuVersionOverlay(TextRenderer& renderer);
    ~MenuVersionOverlay();

    MenuVersionOverlay(const MenuVersionOverlay&) = delete;
    MenuVersionOverlay& operator=(const MenuVersionOverlay&) = delete;
    MenuVersionOverlay(MenuVersionOverlay&&) = delete;
    MenuVersionOverlay& operator=(MenuVersionOverlay&&) = delete;

    void Install();
    void Remove();
    void SetConnectionStatus(ConnectionStatus status) noexcept;

   private:
    void Draw(const TextCanvas& canvas, const TextFrame& frame) const;

    TextRenderer& renderer_;
    std::optional<TextRenderer::CallbackId> callbackId_;
    std::string versionText_;
    std::atomic<ConnectionStatus> status_{ConnectionStatus::Disconnected};
};

}  // namespace sr2ap
