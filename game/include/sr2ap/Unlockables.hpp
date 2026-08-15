#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sr2ap {
    class UnlockableController {
       public:
        UnlockableController();
        ~UnlockableController();

        UnlockableController(const UnlockableController&) = delete;
        UnlockableController& operator=(const UnlockableController&) = delete;

        bool Install(bool blockVanillaRewards, const std::vector<std::string>& managedItems);
        void Remove();
        bool QueueReceivedItem(std::string_view itemName);
        [[nodiscard]] static bool SupportsItem(std::string_view itemName);

       private:
        struct Implementation;
        std::unique_ptr<Implementation> implementation_;
    };
}  // namespace sr2ap
