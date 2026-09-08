#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sr2ap {
class CheatController {
   public:
    using GameThreadTask = std::function<void()>;

    CheatController();
    ~CheatController();

    CheatController(const CheatController&) = delete;
    CheatController& operator=(const CheatController&) = delete;

    bool Install(const std::vector<std::string>& managedItems);
    void Remove();
    bool ActivateReceivedItem(std::string_view itemName);
    bool DispatchOnGameThread(GameThreadTask task);
    [[nodiscard]] static bool SupportsItem(std::string_view itemName);

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};
}  // namespace sr2ap
