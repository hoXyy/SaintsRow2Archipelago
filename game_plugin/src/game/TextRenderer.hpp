#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace sr2ap {

struct TextColor {
    int red;
    int green;
    int blue;
    int alpha;
};

struct TextStyle {
    TextColor color{255, 255, 255, 255};
    // 1.0 is the font's normal size.
    float scale{1.0f};
    // Drawn behind the text at the 8 surrounding offsets.
    std::optional<TextColor> outline;
    int outlineThickness{1};
};

// The screen a draw pass is for. Callbacks run at most once per frame, on
// the screen's last 2D flush, so their text goes above everything else drawn
// through it that frame.
enum class TextScreen : std::uint8_t {
    MainMenu,
    // Pause menu, phone and map, drawn over gameplay.
    InGameMenu,
    // Any other game state on top of the state stack; see TextFrame::gameState.
    GameState,
};

struct TextFrame {
    TextScreen screen;
    // The top game state, e.g. 5 for gameplay. 0 on the main menu.
    std::uint32_t gameState;
};

// Positions are in the game's 1280x720 layout.
class TextCanvas {
   public:
    virtual ~TextCanvas() = default;

    // Text must be ASCII to draw in every language's font.
    virtual void DrawString(const char* text, int x, int y,
                            const TextStyle& style) const = 0;
    // Widescreen layout (the game's own flag); 4:3 otherwise.
    [[nodiscard]] virtual bool IsWidescreen() const = 0;
    // Moves right-anchored x (right half of the layout) with the right edge
    // on ultrawide screens. Left-anchored x is returned unchanged.
    [[nodiscard]] virtual int AnchorX(int x) const = 0;
};

// Owns the 2D flush hook and runs draw callbacks on the game's render thread.
class TextRenderer {
   public:
    // Called on the game thread. Must not add or remove callbacks.
    using DrawCallback =
        std::function<void(const TextCanvas& canvas, const TextFrame& frame)>;
    using CallbackId = std::uint32_t;

    TextRenderer();
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;
    TextRenderer(TextRenderer&&) = delete;
    TextRenderer& operator=(TextRenderer&&) = delete;

    bool Install();
    void Remove();

    // Can be called before Install. Remove waits for a running draw pass.
    CallbackId AddDrawCallback(DrawCallback callback);
    void RemoveDrawCallback(CallbackId id);

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};

}  // namespace sr2ap
