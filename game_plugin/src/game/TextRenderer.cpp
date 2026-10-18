#include "TextRenderer.hpp"

#include <windows.h>

#include <array>
#include <atomic>
#include <cstdint>
#include <exception>
#include <mutex>
#include <optional>
#include <safetyhook.hpp>
#include <string>
#include <utility>
#include <vector>

#include "Addresses.hpp"
#include "GameStateStack.hpp"
#include "Memory.hpp"
#include "ModuleInfo.hpp"
#include "util/HandlerActivity.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
constexpr std::array<std::array<int, 2>, 8> kOutlineOffsets{{
    {-1, -1},
    {0, -1},
    {1, -1},
    {-1, 0},
    {1, 0},
    {-1, 1},
    {0, 1},
    {1, 1},
}};

constexpr float kWidescreenAspectRatio = 16.0f / 9.0f;
constexpr int kVirtualWidth = 1280;
constexpr int kVirtualHeight = 720;

constexpr std::array<std::uint8_t, 6> kExpectedFlushEntry{
    0x8B, 0x15, 0xE8, 0xC4, 0xEA, 0x00,
};

using SetTextColorFunction = void(__cdecl*)(int red, int green, int blue,
                                            int alpha);

template <typename T>
T Read(const std::uintptr_t address) noexcept {
    return *reinterpret_cast<const volatile T*>(address);
}

// Text in ESI, font in EDI, then x, y and scale on the stack; the caller pops
// the stack arguments.
void CallPrintText(const std::uintptr_t function, const char* text,
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
}  // namespace

struct TextRenderer::Implementation {
    Implementation() = default;

    ~Implementation() {
        Remove();
    }

    Implementation(const Implementation&) = delete;
    Implementation& operator=(const Implementation&) = delete;
    Implementation(Implementation&&) = delete;
    Implementation& operator=(Implementation&&) = delete;

    bool Install() {
        if (flushHook) {
            return false;
        }

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
        gameStates.emplace(*game);
        font = base + addresses::kMenuFontRva;
        mainMenuFlag = base + addresses::kMainMenuFlagRva;
        mainMenuShown = base + addresses::kMainMenuShownRva;
        widescreenFlag = base + addresses::kWidescreenFlagRva;
        aspectRatio = base + addresses::kAspectRatioRva;

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
                LogError("TextRenderer", "Failed to disable 2D flush hook");
            }
        }
        handlerActivity.Stop();
        while (!handlerActivity.IsIdle()) {
            SwitchToThread();
        }
        flushHook.reset();
    }

    CallbackId AddDrawCallback(DrawCallback callback) {
        std::lock_guard<std::mutex> lock(callbacksMutex);
        const auto id = ++lastCallbackId;
        callbacks.emplace_back(id, std::move(callback));
        return id;
    }

    void RemoveDrawCallback(const CallbackId id) {
        std::lock_guard<std::mutex> lock(callbacksMutex);
        std::erase_if(callbacks,
                      [id](const auto& entry) { return entry.first == id; });
    }

   private:
    class Canvas final : public TextCanvas {
       public:
        Canvas(const Implementation& owner, const int font)
            : owner_{owner}, font_{font} {
        }

        void DrawString(const char* text, const int x, const int y,
                        const TextStyle& style) const override {
            if (style.outline) {
                owner_.SetTextColor(*style.outline);
                for (const auto& [dx, dy] : kOutlineOffsets) {
                    CallPrintText(owner_.printTextAddress, text, font_,
                                  x + dx * style.outlineThickness,
                                  y + dy * style.outlineThickness, style.scale);
                }
            }
            owner_.SetTextColor(style.color);
            CallPrintText(owner_.printTextAddress, text, font_, x, y,
                          style.scale);
        }

        bool IsWidescreen() const override {
            return Read<std::uint8_t>(owner_.widescreenFlag) != 0;
        }

        int AnchorX(const int x) const override {
            const auto ratio = Read<float>(owner_.aspectRatio);
            if (x < kVirtualWidth / 2 || ratio < kWidescreenAspectRatio) {
                return x;
            }
            // Half the extra width past 16:9, the same way Juiced keeps its
            // version text on the right edge of ultrawide screens.
            const auto extraWidth =
                static_cast<int>(ratio * kVirtualHeight) - kVirtualWidth;
            return x + extraWidth / 2;
        }

       private:
        const Implementation& owner_;
        int font_;
    };

    static void FlushHook(safetyhook::Context& context) {
        const auto handlerLease = handlerActivity.Acquire();
        auto* const self = active.load(std::memory_order_acquire);
        if (handlerLease && self) {
            self->OnFlush(context);
        }
    }

    void OnFlush(const safetyhook::Context& context) {
        const auto frame = ClassifyFlush(context);
        if (!frame) {
            return;
        }
        const auto fontHandle = Read<int>(font);
        // Scaled font handles are negative too, so only -1 means no font.
        if (fontHandle == -1) {
            return;
        }

        const Canvas canvas{*this, fontHandle};
        std::lock_guard<std::mutex> lock(callbacksMutex);
        for (const auto& [id, callback] : callbacks) {
            try {
                callback(canvas, *frame);
            } catch (const std::exception& error) {
                LogError("TextRenderer",
                         std::string{"Draw callback failed: "} + error.what());
            } catch (...) {
                LogError("TextRenderer", "Draw callback failed");
            }
        }
    }

    // The flush runs from the main menu render pass (through Juiced's
    // version-text hook) and from the frame dispatcher, once per state on the
    // state stack, bottom to top, with the state in ESI. Only the last flush
    // of each frame gets a TextFrame.
    std::optional<TextFrame> ClassifyFlush(
        const safetyhook::Context& context) const {
        const auto returnAddress =
            *reinterpret_cast<const std::uintptr_t*>(context.esp);
        if (returnAddress != dispatcherFlushReturn) {
            if (Read<std::uint8_t>(mainMenuFlag) == 1 &&
                Read<std::uint8_t>(mainMenuShown) == 1) {
                return TextFrame{TextScreen::MainMenu, 0};
            }
            return std::nullopt;
        }

        const auto topIndex = gameStates->TopIndex();
        const auto topState = gameStates->StateAt(topIndex);
        if (!topState) {
            return std::nullopt;
        }
        const auto dispatchedState = static_cast<std::uint32_t>(context.esi);
        // The in-game menu state is never flushed itself, so the layer right
        // under it is the last flush of the frame.
        if (*topState == addresses::kInGameMenuGameState) {
            if (gameStates->StateAt(topIndex - 1) == dispatchedState) {
                return TextFrame{TextScreen::InGameMenu, *topState};
            }
            return std::nullopt;
        }
        if (*topState == dispatchedState) {
            return TextFrame{TextScreen::GameState, *topState};
        }
        return std::nullopt;
    }

    void SetTextColor(const TextColor& color) const {
        reinterpret_cast<SetTextColorFunction>(setTextColorAddress)(
            color.red, color.green, color.blue, color.alpha);
    }

    std::uintptr_t printTextAddress{};
    std::uintptr_t setTextColorAddress{};
    std::uintptr_t dispatcherFlushReturn{};
    std::optional<GameStateStack> gameStates;
    std::uintptr_t font{};
    std::uintptr_t mainMenuFlag{};
    std::uintptr_t mainMenuShown{};
    std::uintptr_t widescreenFlag{};
    std::uintptr_t aspectRatio{};
    safetyhook::MidHook flushHook;
    std::mutex callbacksMutex;
    std::vector<std::pair<CallbackId, DrawCallback>> callbacks;
    CallbackId lastCallbackId{};
    inline static HandlerActivity handlerActivity;
    inline static std::atomic<Implementation*> active{};
};

TextRenderer::TextRenderer()
    : implementation_{std::make_unique<Implementation>()} {
}

TextRenderer::~TextRenderer() = default;

bool TextRenderer::Install() {
    return implementation_->Install();
}

void TextRenderer::Remove() {
    implementation_->Remove();
}

TextRenderer::CallbackId TextRenderer::AddDrawCallback(DrawCallback callback) {
    return implementation_->AddDrawCallback(std::move(callback));
}

void TextRenderer::RemoveDrawCallback(const CallbackId id) {
    implementation_->RemoveDrawCallback(id);
}

}  // namespace sr2ap
