#pragma once

#include <memory>
#include <string_view>

namespace sr2ap {
    class NotorietyController {
       public:
        NotorietyController();
        ~NotorietyController();

        NotorietyController(const NotorietyController&) = delete;
        NotorietyController& operator=(const NotorietyController&) = delete;

        bool Install();
        void Remove();
        bool ActivateReceivedItem(std::string_view itemName) const;

       private:
        struct Implementation;
        std::unique_ptr<Implementation> implementation_;
    };
}  // namespace sr2ap
