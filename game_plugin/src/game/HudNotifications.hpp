#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

namespace sr2ap {

class GameThreadDispatcher;

class NotificationText {
   public:
    NotificationText& Append(std::string_view text);
    // RGB is provided in hex.
    NotificationText& Append(std::string_view text, std::uint32_t rgb);

    [[nodiscard]] const std::string& Markup() const noexcept {
        return markup_;
    }

   private:
    std::string markup_;
};

class HudNotifications {
   public:
    explicit HudNotifications(GameThreadDispatcher& dispatcher);
    ~HudNotifications();

    HudNotifications(const HudNotifications&) = delete;
    HudNotifications& operator=(const HudNotifications&) = delete;
    HudNotifications(HudNotifications&&) = delete;
    HudNotifications& operator=(HudNotifications&&) = delete;

    bool Install();
    void Remove();
    // Any thread.
    void Post(const NotificationText& text);
    // Plugin thread, once per loop: hands queued messages to the game thread.
    void Update();

   private:
    struct Implementation;
    GameThreadDispatcher& dispatcher_;
    std::shared_ptr<Implementation> implementation_;
};

}  // namespace sr2ap
