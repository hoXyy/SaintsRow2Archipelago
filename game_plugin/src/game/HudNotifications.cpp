#include "HudNotifications.hpp"

#include <cstdint>
#include <cstdio>
#include <deque>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "Addresses.hpp"
#include "GameStateStack.hpp"
#include "GameThreadDispatcher.hpp"
#include "Memory.hpp"
#include "ModuleInfo.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
// Oldest messages are dropped past this while the HUD isn't up.
constexpr std::size_t kMaximumQueuedNotifications = 256;
// Keeps one Lua string small during a big burst; the rest follow on the
// next updates. hud_msg.lua queues them for display either way.
constexpr std::size_t kMaximumNotificationsPerFlush = 16;

void AppendDisplayText(std::string& out, const std::string_view text) {
    for (const auto character : text) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte == '[') {
            out += '(';
        } else if (byte == ']') {
            out += ')';
        } else if (byte < 0x20 || byte == 0x7F) {
            out += ' ';
        } else if (byte < 0x80) {
            out += character;
        } else if ((byte & 0xC0) != 0x80) {
            // Leading byte of a UTF-8 sequence; its continuation bytes are
            // dropped.
            out += '?';
        }
    }
}

void AppendLuaString(std::string& out, const std::string_view text) {
    out += '"';
    for (const auto character : text) {
        const auto byte = static_cast<unsigned char>(character);
        if (character == '"' || character == '\\') {
            out += '\\';
            out += character;
        } else if (byte < 0x20 || byte >= 0x7F) {
            char escaped[5];
            std::snprintf(escaped, sizeof(escaped), "\\%03u", byte);
            out += escaped;
        } else {
            out += character;
        }
    }
    out += '"';
}

void CallVintLuaExecute(const std::uintptr_t function,
                        const std::uintptr_t state, const char* code) {
    __asm {
            push esi
            mov esi, state
            mov eax, code
            call function
            pop esi
    }
}
}  // namespace

NotificationText& NotificationText::Append(const std::string_view text) {
    AppendDisplayText(markup_, text);
    return *this;
}

NotificationText& NotificationText::Append(const std::string_view text,
                                           const std::uint32_t rgb) {
    char color[32];
    std::snprintf(color, sizeof(color), "[format][color:#%06X]",
                  static_cast<unsigned>(rgb & 0xFFFFFF));
    markup_ += color;
    AppendDisplayText(markup_, text);
    markup_ += "[/format]";
    return *this;
}

struct HudNotifications::Implementation {
    // Game thread. Sends the next batch to hud_msg.lua, but only while
    // gameplay is on top: sr2ap_msg_add only exists while the gameplay HUD
    // (and hud_msg.lua with it) is loaded.
    void Flush() {
        std::string code;
        std::uintptr_t luaState{};
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (!installed || queue.empty() || !gameStates->IsGameplayOnTop()) {
                return;
            }
            luaState =
                *reinterpret_cast<const volatile std::uintptr_t*>(stateAddress);
            if (luaState == 0) {
                return;
            }
            for (std::size_t count = 0;
                 count < kMaximumNotificationsPerFlush && !queue.empty();
                 ++count) {
                code += "sr2ap_msg_add(";
                AppendLuaString(code, queue.front());
                code += ")\n";
                queue.pop_front();
            }
        }
        CallVintLuaExecute(executeAddress, luaState, code.c_str());
    }

    std::mutex mutex;
    std::deque<std::string> queue;
    std::optional<GameStateStack> gameStates;
    std::uintptr_t executeAddress{};
    std::uintptr_t stateAddress{};
    bool installed{};
};

HudNotifications::HudNotifications(GameThreadDispatcher& dispatcher)
    : dispatcher_{dispatcher},
      implementation_{std::make_shared<Implementation>()} {
}

HudNotifications::~HudNotifications() {
    Remove();
}

bool HudNotifications::Install() {
    const auto game = InspectSupportedGameModule();
    if (!game) {
        return false;
    }
    const auto executeAddress = game->base + addresses::kVintLuaExecuteRva;
    if (!ValidateFunctionTarget(*game, executeAddress)) {
        return false;
    }

    std::lock_guard<std::mutex> lock(implementation_->mutex);
    implementation_->gameStates.emplace(*game);
    implementation_->executeAddress = executeAddress;
    implementation_->stateAddress = game->base + addresses::kVintLuaStateRva;
    implementation_->installed = true;
    return true;
}

void HudNotifications::Remove() {
    std::lock_guard<std::mutex> lock(implementation_->mutex);
    implementation_->installed = false;
    implementation_->queue.clear();
}

void HudNotifications::Post(const NotificationText& text) {
    if (text.Markup().empty()) {
        return;
    }
    std::lock_guard<std::mutex> lock(implementation_->mutex);
    auto& queue = implementation_->queue;
    if (queue.size() == kMaximumQueuedNotifications) {
        queue.pop_front();
        LogWarning("Notifications", "Queue full; dropped the oldest");
    }
    queue.push_back(text.Markup());
}

void HudNotifications::Update() {
    {
        std::lock_guard<std::mutex> lock(implementation_->mutex);
        if (!implementation_->installed || implementation_->queue.empty()) {
            return;
        }
    }
    dispatcher_.Dispatch([state = implementation_] { state->Flush(); });
}

}  // namespace sr2ap
