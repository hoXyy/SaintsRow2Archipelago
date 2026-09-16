#pragma once

#include <cstdint>
#include <memory>

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
    bool GrantBar();
    void Update();

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};
}  // namespace sr2ap
