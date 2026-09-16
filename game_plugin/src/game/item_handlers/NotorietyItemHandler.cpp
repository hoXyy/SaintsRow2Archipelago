#include "NotorietyItemHandler.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <utility>

#include "game/GameThreadDispatcher.hpp"
#include "game/Memory.hpp"
#include "game/ModuleInfo.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {

inline constexpr std::ptrdiff_t kNotorietySetRva = 0x0015F870;

struct NotorietyTrap {
    std::string_view itemName;
    int faction_id;
};

constexpr std::array<NotorietyTrap, 4> supportedItems{{
    {.itemName = "Trap: Max Police Notoriety", .faction_id = 3},
    {.itemName = "Trap: Max Ronin Notoriety", .faction_id = 1},
    {.itemName = "Trap: Max Brotherhood Notoriety", .faction_id = 0},
    {.itemName = "Trap: Max Samedi Notoriety", .faction_id = 2},
}};
constexpr float maximumNotorietyLevel{5.0F};
constexpr float noNotoriety{0.0F};

const NotorietyTrap* FindItem(const std::string_view itemName) noexcept {
    const auto found = std::ranges::find_if(
        supportedItems,
        [itemName](const auto& trap) { return trap.itemName == itemName; });
    return found == supportedItems.end() ? nullptr : &*found;
}

class NotorietyItemHandler final : public ItemHandler {
   public:
    explicit NotorietyItemHandler(GameThreadDispatcher& dispatcher)
        : dispatcher_(dispatcher) {
    }

    bool Install() override {
        const auto game = InspectSupportedGameModule();
        if (!game) {
            return false;
        }

        setAddress_ = game->base + kNotorietySetRva;
        constexpr std::array<std::uint8_t, 6> expectedSet{0x81, 0xEC, 0x2C,
                                                          0x05, 0x00, 0x00};
        const auto actualSet =
            ReadMemoryIntoArray<std::uint8_t, expectedSet.size()>(setAddress_);

        installed_ =
            IsInsideModule(game->handle,
                           reinterpret_cast<const void*>(setAddress_)) &&
            IsExecutableAddress(setAddress_) &&
            DetectDetour(reinterpret_cast<const void*>(setAddress_)) ==
                DetourKind::None &&
            actualSet && *actualSet == expectedSet;
        return installed_;
    }

    [[nodiscard]] bool CanHandleItem(
        const std::string_view itemName) const noexcept override {
        return FindItem(itemName) != nullptr;
    }

    bool Handle(const std::string_view itemName) override {
        const auto* const trap = FindItem(itemName);
        if (!trap) {
            return false;
        }

        if (!installed_) {
            LogWarning(
                "Notoriety",
                "Rejected trap while native integration is unavailable: " +
                    std::string{itemName});
            return false;
        }

        const auto nativeSetAddress = setAddress_;
        const int factionId = trap->faction_id;
        const bool isPolice = factionId == 3;
        std::string ownedItemName{itemName};

        const bool queued =
            dispatcher_.Dispatch([nativeSetAddress, factionId, isPolice,
                                  itemName = std::move(ownedItemName)] {
                const auto setNotoriety =
                    reinterpret_cast<void(__cdecl*)(int, float)>(
                        nativeSetAddress);

                // trap doesn't work if the player already has notoriety, so
                // need to set it to 0 before setting it to max for police but
                // for a gang need to set all gang notoriety to 0 first
                if (isPolice) {
                    setNotoriety(factionId, noNotoriety);
                } else {
                    setNotoriety(0, noNotoriety);
                    setNotoriety(1, noNotoriety);
                    setNotoriety(2, noNotoriety);
                }

                setNotoriety(factionId, maximumNotorietyLevel);

                LogInfo("Notoriety",
                        "Activated trap item=" + itemName +
                            " faction=" + std::to_string(factionId));
            });

        if (!queued) {
            LogWarning("Notoriety", "Unable to queue trap for game thread: " +
                                        std::string{itemName});
        }

        return queued;
    }

    void Remove() noexcept override {
        setAddress_ = 0;
        installed_ = false;
    }

   private:
    bool installed_{};
    std::uintptr_t setAddress_{};
    GameThreadDispatcher& dispatcher_;
};
}  // namespace

ItemHandlerPtr CreateNotorietyItemHandler(GameThreadDispatcher& dispatcher) {
    return std::make_unique<NotorietyItemHandler>(dispatcher);
}
}  // namespace sr2ap
