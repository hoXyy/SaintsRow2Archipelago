#include "MenuVersionOverlay.hpp"

#include <windows.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <safetyhook.hpp>
#include <string>
#include <string_view>
#include <utility>

#include "Addresses.hpp"
#include "Memory.hpp"
#include "ModuleInfo.hpp"
#include "util/HandlerActivity.hpp"
#include "util/Logger.hpp"

#ifndef SR2AP_VERSION
#define SR2AP_VERSION "dev"
#endif

namespace sr2ap {
namespace {
struct TextColor {
    int red;
    int green;
    int blue;
    int alpha;
};

struct StatusLine {
    const char* text;
    TextColor color;
};

constexpr std::string_view kVersionPrefix = "SR2AP ";
constexpr TextColor kVersionColor{160, 160, 160, 128};
constexpr std::array<StatusLine, 4> kStatusLines{{
    {"Status: Client disabled", {160, 160, 160, 128}},
    {"Status: Not connected", {220, 80, 80, 160}},
    {"Status: Waiting for session", {230, 200, 80, 160}},
    {"Status: Connected", {110, 210, 110, 160}},
}};

constexpr float kTextScale = 0.6f;
constexpr int kLineHeight = 17;
constexpr int kMainMenuWidescreenX = 10;
constexpr int kMainMenuWidescreenY = 700 - 2 * kLineHeight;
constexpr int kMainMenuX = 0;
constexpr int kMainMenuY = kLineHeight;
constexpr int kPauseMenuX = 160;
constexpr int kPauseMenuY = 680 - kLineHeight;

constexpr float kWidescreenAspectRatio = 16.0f / 9.0f;
constexpr int kVirtualWidth = 1280;
constexpr int kVirtualHeight = 720;
constexpr int kMaxGameStateDepth = 16;

constexpr std::array<std::uint8_t, 6> kExpectedFlushEntry{
    0x8B, 0x15, 0xE8, 0xC4, 0xEA, 0x00,
};

using SetTextColorFunction = void(__cdecl*)(int red, int green, int blue,
                                            int alpha);

std::string BuildVersionText() {
    return std::string{kVersionPrefix} + SR2AP_VERSION;
}
}  // namespace

struct MenuVersionOverlay::Implementation {
    Implementation() = default;

    ~Implementation() {
        Remove();
    }

    Implementation(const Implementation&) = delete;
    Implementation& operator=(const Implementation&) = delete;
    Implementation(Implementation&&) = delete;
    Implementation& operator=(Implementation&&) = delete;

    bool Install() {
        const auto game = InspectSupportedGameModule();
        if (!game) {
            return false;
        }

        const auto base = game->base;
        const auto flushAddress = base + addresses::kMenuTextFlushRva;
        printTextAddress = base + addresses::kPrintScaledTextRva;
        setTextColorAddress = base + addresses::kSetTextColorRva;
        if (!ValidateHookTarget(*game, flushAddress, kExpectedFlushEntry) ||
            !ValidateFunctionTarget(*game, printTextAddress) ||
            !ValidateFunctionTarget(*game, setTextColorAddress)) {
            return false;
        }

        dispatcherFlushReturn =
            base + addresses::kStateDispatcherFlushReturnRva;
        gameStateStack = base + addresses::kGameStateStackRva;
        gameStateTopIndex = base + addresses::kGameStateTopIndexRva;
        menuFont = base + addresses::kMenuFontRva;
        mainMenuFlag = base + addresses::kMainMenuFlagRva;
        mainMenuShown = base + addresses::kMainMenuShownRva;
        menuState = base + addresses::kMenuStateRva;
        widescreenFlag = base + addresses::kWidescreenFlagRva;
        aspectRatio = base + addresses::kAspectRatioRva;
        versionText = BuildVersionText();

        auto hook = safetyhook::MidHook::create(
            reinterpret_cast<void*>(flushAddress), &FlushHook,
            safetyhook::MidHook::StartDisabled);
        if (!hook) {
            return false;
        }

        flushHook = std::move(*hook);
        handlerActivity.Start();
        active.store(this, std::memory_order_release);
        if (const auto enabled = flushHook.enable(); !enabled) {
            active.store(nullptr, std::memory_order_release);
            flushHook.reset();
            return false;
        }

        return true;
    }

    void Remove() {
        active.store(nullptr, std::memory_order_release);
        if (flushHook && flushHook.enabled()) {
            if (const auto disabled = flushHook.disable(); !disabled) {
                LogError("MenuOverlay", "Failed to disable menu text hook");
            }
        }
        handlerActivity.Stop();
        while (!handlerActivity.IsIdle()) {
            SwitchToThread();
        }
        flushHook.reset();
    }

    std::atomic<ConnectionStatus> status{ConnectionStatus::Disconnected};

   private:
    static void FlushHook(safetyhook::Context& context) {
        const auto handlerLease = handlerActivity.Acquire();
        auto* const self = active.load(std::memory_order_acquire);
        if (handlerLease && self) {
            self->Draw(context);
        }
    }

    void Draw(const safetyhook::Context& context) const {
        const auto returnAddress =
            *reinterpret_cast<const std::uintptr_t*>(context.esp);
        if (returnAddress == dispatcherFlushReturn) {
            if (IsPauseMenuTopState(context.esi)) {
                DrawLines(kPauseMenuX, kPauseMenuY);
            }
            return;
        }

        if (Read<std::uint8_t>(mainMenuFlag) != 1 ||
            Read<std::uint8_t>(mainMenuShown) != 1) {
            return;
        }
        if (Read<std::uint8_t>(widescreenFlag) != 0) {
            DrawLines(WidescreenRightX(kMainMenuWidescreenX),
                      kMainMenuWidescreenY);
        } else {
            DrawLines(kMainMenuX, kMainMenuY);
        }
    }

    // The dispatcher flushes once per state on the stack, bottom to top, with
    // the state in ESI. Drawing only for the top state draws once per frame,
    // above the layers below it.
    bool IsPauseMenuTopState(const std::uintptr_t dispatchedState) const {
        const auto state = Read<std::uint8_t>(menuState);
        if (state < addresses::kPauseMenuFirstState ||
            state > addresses::kPauseMenuLastState) {
            return false;
        }
        const auto topIndex = Read<int>(gameStateTopIndex);
        if (topIndex < 0 || topIndex >= kMaxGameStateDepth) {
            return false;
        }
        const auto topState = Read<std::uint32_t>(
            gameStateStack +
            static_cast<std::uintptr_t>(topIndex) * sizeof(std::uint32_t));
        return topState == dispatchedState;
    }

    // Moves x right by half the extra width past 16:9, the same way Juiced
    // keeps its version text on the right edge of ultrawide screens.
    int WidescreenRightX(const int x) const {
        const auto ratio = Read<float>(aspectRatio);
        if (ratio < kWidescreenAspectRatio) {
            return x;
        }
        const auto extraWidth =
            static_cast<int>(ratio * kVirtualHeight) - kVirtualWidth;
        return x + extraWidth / 2;
    }

    void DrawLines(const int x, const int y) const {
        const auto font = Read<int>(menuFont);
        // Scaled font handles are negative too, so only -1 means no font.
        if (font == -1) {
            return;
        }
        DrawText(versionText.c_str(), kVersionColor, font, x, y);

        const auto index =
            static_cast<std::size_t>(status.load(std::memory_order_relaxed));
        if (index < kStatusLines.size()) {
            const auto& line = kStatusLines[index];
            DrawText(line.text, line.color, font, x, y + kLineHeight);
        }
    }

    void DrawText(const char* text, const TextColor& color, const int font,
                  const int x, const int y) const {
        reinterpret_cast<SetTextColorFunction>(setTextColorAddress)(
            color.red, color.green, color.blue, color.alpha);
        CallPrintText(printTextAddress, text, font, x, y, kTextScale);
    }

    // Text in ESI, font in EDI, then x, y and scale on the stack; the caller
    // pops the stack arguments.
    static void CallPrintText(const std::uintptr_t function, const char* text,
                              const int font, const int x, const int y,
                              const float scale) {
        __asm {
                push esi
                push edi
                mov esi, text
                mov edi, font
                push scale
                push y
                push x
                call function
                add esp, 12
                pop edi
                pop esi
        }
    }

    template <typename T>
    static T Read(const std::uintptr_t address) noexcept {
        return *reinterpret_cast<const volatile T*>(address);
    }

    std::uintptr_t printTextAddress{};
    std::uintptr_t setTextColorAddress{};
    std::uintptr_t dispatcherFlushReturn{};
    std::uintptr_t gameStateStack{};
    std::uintptr_t gameStateTopIndex{};
    std::uintptr_t menuFont{};
    std::uintptr_t mainMenuFlag{};
    std::uintptr_t mainMenuShown{};
    std::uintptr_t menuState{};
    std::uintptr_t widescreenFlag{};
    std::uintptr_t aspectRatio{};
    std::string versionText;
    safetyhook::MidHook flushHook;
    inline static HandlerActivity handlerActivity;
    inline static std::atomic<Implementation*> active{};
};

MenuVersionOverlay::MenuVersionOverlay() = default;

MenuVersionOverlay::~MenuVersionOverlay() {
    Remove();
}

bool MenuVersionOverlay::Install() {
    if (implementation_) {
        return false;
    }

    implementation_ = std::make_unique<Implementation>();
    if (!implementation_->Install()) {
        implementation_.reset();
        return false;
    }

    return true;
}

void MenuVersionOverlay::Remove() {
    implementation_.reset();
}

void MenuVersionOverlay::SetConnectionStatus(
    const ConnectionStatus status) noexcept {
    if (implementation_) {
        implementation_->status.store(status, std::memory_order_relaxed);
    }
}

}  // namespace sr2ap
