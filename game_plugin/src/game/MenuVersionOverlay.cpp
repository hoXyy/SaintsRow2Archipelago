#include "MenuVersionOverlay.hpp"

#include <array>
#include <cstddef>
#include <string>
#include <string_view>

#ifndef SR2AP_VERSION
#define SR2AP_VERSION "dev"
#endif

namespace sr2ap {
namespace {
struct StatusLine {
    const char* text;
    TextColor color;
};

constexpr std::string_view kVersionPrefix = "SR2AP ";
constexpr TextColor kVersionColor{160, 160, 160, 255};
constexpr std::array<StatusLine, 4> kStatusLines{{
    {"Status: Client disabled", {160, 160, 160, 128}},
    {"Status: Not connected", {220, 80, 80, 255}},
    {"Status: Waiting for session", {230, 200, 80, 255}},
    {"Status: Connected", {110, 210, 110, 255}},
}};

constexpr float kTextScale = 0.6f;
constexpr TextColor kOutlineColor{0, 0, 0, 160};
constexpr int kOutlineThickness = 1;
constexpr int kLineHeight = 17;
// Used for both the main menu and the in-game menu. On ultrawide screens, x
// in the right half of the 1280-wide layout moves with the right edge.
constexpr int kWidescreenX = 10;
constexpr int kWidescreenY = 700 - 2 * kLineHeight;
constexpr int kNarrowX = 0;
constexpr int kNarrowY = kLineHeight;

TextStyle MakeStyle(const TextColor& color) {
    return TextStyle{
        .color = color,
        .scale = kTextScale,
        .outline = kOutlineColor,
        .outlineThickness = kOutlineThickness,
    };
}
}  // namespace

MenuVersionOverlay::MenuVersionOverlay(TextRenderer& renderer)
    : renderer_{renderer},
      versionText_{std::string{kVersionPrefix} + SR2AP_VERSION} {
}

MenuVersionOverlay::~MenuVersionOverlay() {
    Remove();
}

void MenuVersionOverlay::Install() {
    if (callbackId_) {
        return;
    }
    callbackId_ = renderer_.AddDrawCallback(
        [this](const TextCanvas& canvas, const TextFrame& frame) {
            Draw(canvas, frame);
        });
}

void MenuVersionOverlay::Remove() {
    if (callbackId_) {
        renderer_.RemoveDrawCallback(*callbackId_);
        callbackId_.reset();
    }
}

void MenuVersionOverlay::SetConnectionStatus(
    const ConnectionStatus status) noexcept {
    status_.store(status, std::memory_order_relaxed);
}

void MenuVersionOverlay::Draw(const TextCanvas& canvas,
                              const TextFrame& frame) const {
    if (frame.screen != TextScreen::MainMenu &&
        frame.screen != TextScreen::InGameMenu) {
        return;
    }

    const bool widescreen = canvas.IsWidescreen();
    const auto x = widescreen ? canvas.AnchorX(kWidescreenX) : kNarrowX;
    const auto y = widescreen ? kWidescreenY : kNarrowY;
    canvas.DrawString(versionText_.c_str(), x, y, MakeStyle(kVersionColor));

    const auto index =
        static_cast<std::size_t>(status_.load(std::memory_order_relaxed));
    if (index < kStatusLines.size()) {
        const auto& line = kStatusLines[index];
        canvas.DrawString(line.text, x, y + kLineHeight, MakeStyle(line.color));
    }
}

}  // namespace sr2ap
