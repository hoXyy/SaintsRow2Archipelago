#pragma once

#include <cstdint>
#include <memory>
#include <string_view>

namespace sr2ap {
    class RespectController {
       public:
        RespectController();
        ~RespectController();

        RespectController(const RespectController&) = delete;
        RespectController& operator=(const RespectController&) = delete;

        bool Install();
        void PermitNextSaveRestore(std::uint32_t threadId) noexcept;
        void Remove();
        bool ActivateReceivedItem(std::string_view itemName);
        void Update();

       private:
        struct Implementation;
        std::unique_ptr<Implementation> implementation_;
    };
}  // namespace sr2ap
