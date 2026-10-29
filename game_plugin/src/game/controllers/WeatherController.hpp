#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "util/GameEvents.hpp"

namespace sr2ap {
class GameThreadDispatcher;

class WeatherController {
   public:
    WeatherController();
    ~WeatherController();

    WeatherController(const WeatherController&) = delete;
    WeatherController& operator=(const WeatherController&) = delete;

    bool Install(const std::vector<std::string>& managedItems);
    void Remove();
    bool ActivateReceivedItem(std::string_view itemName,
                              GameThreadDispatcher& dispatcher,
                              ItemCompletion completion);
    [[nodiscard]] static bool SupportsItem(std::string_view itemName);

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};
}  // namespace sr2ap
