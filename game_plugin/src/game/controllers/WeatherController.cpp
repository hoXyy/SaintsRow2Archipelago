#include "WeatherController.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "game/Addresses.hpp"
#include "game/GameThreadDispatcher.hpp"
#include "game/Memory.hpp"
#include "game/ModuleInfo.hpp"
#include "util/Logger.hpp"

namespace sr2ap {
namespace {
struct WeatherDefinition {
    std::string_view itemName;
    std::string_view stageName;
};

constexpr std::array<WeatherDefinition, 5> supportedApItems{{
    {"Clear Skies Weather", "Clear Skies"},
    {"Overcast Weather", "Overcast"},
    {"Light Rain", "Light Rain"},
    {"Heavy Rain", "Thunderstorm"},
    {"Thunderstorm", "Thunderstorm"},
}};

// Frames to wait for the weather tick to take the stage before letting go.
constexpr std::uint32_t stageReleaseMaxFrames{300};

const WeatherDefinition* FindWeather(const std::string_view itemName) {
    const auto found =
        std::ranges::find_if(supportedApItems, [itemName](const auto& weather) {
            return weather.itemName == itemName;
        });
    return found == supportedApItems.end() ? nullptr : &*found;
}

bool EqualsIgnoringCase(const std::string_view left,
                        const std::string_view right) {
    return std::ranges::equal(left, right, [](const char a, const char b) {
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    });
}
}  // namespace

struct WeatherController::Implementation {
    bool Install(const std::vector<std::string>& managedItems) {
        const auto game = InspectSupportedGameModule();
        if (!game) {
            return false;
        }

        gameBase = game->base;
        managedItemNames.clear();
        managedItemNames.insert(managedItems.begin(), managedItems.end());
        installed = true;
        return true;
    }

    void Remove() {
        installed = false;
    }

    bool ActivateReceivedItem(const std::string_view itemName,
                              GameThreadDispatcher& dispatcher,
                              ItemCompletion completion) const {
        if (!installed) {
            return false;
        }

        const auto* const definition = FindWeather(itemName);
        if (!definition || !managedItemNames.contains(std::string{itemName})) {
            return false;
        }

        const auto stage = FindStage(gameBase, definition->stageName);
        if (!stage) {
            LogWarning("Weather", "Weather stage not found: " +
                                      std::string{definition->stageName});
            return false;
        }

        const auto base = gameBase;
        const bool queued = dispatcher.Dispatch([base, stage = *stage,
                                                 &dispatcher, completion] {
            if (!completion.CanApply()) {
                return;
            }
            *reinterpret_cast<volatile std::uint32_t*>(
                base + addresses::kWeatherBashStageRva) = stage;
            completion.Complete(true);
            QueueStageRelease(dispatcher, base, stage, stageReleaseMaxFrames);
        });

        if (!queued) {
            LogWarning("Weather",
                       "Failed to queue received item for game thread: " +
                           std::string{itemName});
            return false;
        }

        LogDebug("Weather", "Queued received item for game thread: " +
                                std::string{itemName});

        return true;
    }

   private:
    // What the game's own lookup does: stage names compare ignoring case.
    static std::optional<std::uint32_t> FindStage(const std::uintptr_t base,
                                                  const std::string_view name) {
        const auto table =
            ReadMemory<std::uint32_t>(base + addresses::kWeatherStageTableRva);
        const auto count =
            ReadMemory<std::int32_t>(base + addresses::kWeatherStageCountRva);
        if (!table || !count || *table == 0) {
            return std::nullopt;
        }

        for (std::int32_t index = 0; index < *count; ++index) {
            const auto stage = *table + static_cast<std::uint32_t>(index) *
                                            addresses::kWeatherStageSize;
            const auto namePointer = ReadMemory<std::uint32_t>(stage);
            if (!namePointer) {
                continue;
            }
            const auto stageName = ReadFixedString(*namePointer, 32);
            if (stageName && EqualsIgnoringCase(*stageName, name)) {
                return stage;
            }
        }
        return std::nullopt;
    }

    // The tick keeps refilling the stage slots while the bash stage is set,
    // so it is cleared once the tick has taken it. Runs on the game thread;
    // tasks dispatched from a task run next frame.
    static void QueueStageRelease(GameThreadDispatcher& dispatcher,
                                  const std::uintptr_t base,
                                  const std::uint32_t stage,
                                  const std::uint32_t framesLeft) {
        const bool queued =
            dispatcher.Dispatch([&dispatcher, base, stage, framesLeft] {
                auto* const bashStage =
                    reinterpret_cast<volatile std::uint32_t*>(
                        base + addresses::kWeatherBashStageRva);
                if (*bashStage != stage) {
                    // A later weather item took over.
                    return;
                }
                const auto slots = ReadMemory<std::uint32_t>(
                    base + addresses::kWeatherStageSlotsPointerRva);
                const auto shown = slots && *slots != 0
                                       ? ReadMemory<std::uint32_t>(*slots)
                                       : std::nullopt;
                if (framesLeft > 0 && (!shown || *shown != stage)) {
                    QueueStageRelease(dispatcher, base, stage, framesLeft - 1);
                    return;
                }
                *bashStage = 0;
            });

        if (!queued) {
            LogWarning("Weather", "Failed to queue weather stage release");
        }
    }

    std::uintptr_t gameBase{};
    std::unordered_set<std::string> managedItemNames;
    bool installed{};
};

WeatherController::WeatherController() = default;

WeatherController::~WeatherController() {
    Remove();
}

bool WeatherController::Install(const std::vector<std::string>& managedItems) {
    if (implementation_) {
        return false;
    }

    implementation_ = std::make_unique<Implementation>();
    if (!implementation_->Install(managedItems)) {
        implementation_.reset();
        return false;
    }

    return true;
}

void WeatherController::Remove() {
    implementation_.reset();
}

bool WeatherController::ActivateReceivedItem(const std::string_view itemName,
                                             GameThreadDispatcher& dispatcher,
                                             ItemCompletion completion) {
    return implementation_ && implementation_->ActivateReceivedItem(
                                  itemName, dispatcher, std::move(completion));
}

bool WeatherController::SupportsItem(const std::string_view itemName) {
    return FindWeather(itemName) != nullptr;
}

}  // namespace sr2ap
