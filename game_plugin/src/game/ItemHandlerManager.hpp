#pragma once

#include <cstdint>
#include <string_view>

#include "GameThreadDispatcher.hpp"
#include "ItemHandler.hpp"
#include "ItemHandlerRegistry.hpp"
#include "controllers/RespectController.hpp"

namespace sr2ap {

class ItemHandlerManager {
   public:
    ItemHandlerManager() = default;
    ~ItemHandlerManager();

    ItemHandlerManager(const ItemHandlerManager&) = delete;
    ItemHandlerManager& operator=(const ItemHandlerManager&) = delete;
    ItemHandlerManager(ItemHandlerManager&&) = delete;
    ItemHandlerManager& operator=(ItemHandlerManager&&) = delete;

    [[nodiscard]] bool Install(ItemHandlerConfiguration configuration,
                               bool saveMonitoringAvailable);
    [[nodiscard]] bool Handle(std::string_view itemName) const;
    void PermitNextSaveRestore(std::uint32_t threadId) const noexcept;
    void Update() const;
    void Remove();

   private:
    GameThreadDispatcher dispatcher_;
    RespectController respectController_;
    ItemHandlers handlers_;
    bool installed_{};
};

}  // namespace sr2ap
