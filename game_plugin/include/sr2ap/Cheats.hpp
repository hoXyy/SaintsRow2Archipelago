#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sr2ap {
    class CheatController {
       public:
        CheatController();
        ~CheatController();

        CheatController(const CheatController&) = delete;
        CheatController& operator=(const CheatController&) = delete;

        bool Install(const std::vector<std::string>& managedItems);
        void Remove();
        bool ActivateReceivedItem(std::string_view itemName);
        [[nodiscard]] static bool SupportsItem(std::string_view itemName);

       private:
        struct Implementation;
        std::unique_ptr<Implementation> implementation_;
    };
}  // namespace sr2ap
