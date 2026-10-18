#pragma once

#include <cstdint>
#include <memory>

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
    MenuVersionOverlay();
    ~MenuVersionOverlay();

    MenuVersionOverlay(const MenuVersionOverlay&) = delete;
    MenuVersionOverlay& operator=(const MenuVersionOverlay&) = delete;
    MenuVersionOverlay(MenuVersionOverlay&&) = delete;
    MenuVersionOverlay& operator=(MenuVersionOverlay&&) = delete;

    bool Install();
    void Remove();
    void SetConnectionStatus(ConnectionStatus status) noexcept;

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

}  // namespace sr2ap
