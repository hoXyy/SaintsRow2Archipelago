#include "ItemHandlerManager.hpp"

#include <utility>

namespace sr2ap {

ItemHandlerManager::~ItemHandlerManager() {
    Remove();
}

bool ItemHandlerManager::Install(ItemHandlerConfiguration configuration,
                                 const bool saveMonitoringAvailable) {
    if (installed_ ||
        (configuration.exclusiveRespect && !saveMonitoringAvailable)) {
        return false;
    }

    const bool needsGameThread =
        !configuration.managedCheats.empty() || configuration.notorietyTraps;
    if (needsGameThread && !dispatcher_.Install()) {
        return false;
    }

    handlers_ = CreateItemHandlers(std::move(configuration), dispatcher_,
                                   respectController_);
    for (const auto& handler : handlers_) {
        if (!handler->Install()) {
            Remove();
            return false;
        }
    }

    installed_ = true;
    return true;
}

bool ItemHandlerManager::Handle(const std::string_view itemName) const {
    for (auto& handler : handlers_) {
        if (handler->CanHandleItem(itemName)) {
            return handler->Handle(itemName);
        }
    }
    return false;
}

void ItemHandlerManager::PermitNextSaveRestore(
    const std::uint32_t threadId) const noexcept {
    respectController_.PermitNextSaveRestore(threadId);
}

void ItemHandlerManager::Update() const {
    for (auto& handler : handlers_) {
        handler->Update();
    }
}

void ItemHandlerManager::Remove() {
    dispatcher_.Remove();
    for (auto handler = handlers_.rbegin(); handler != handlers_.rend();
         ++handler) {
        (*handler)->Remove();
    }
    handlers_.clear();
    installed_ = false;
}

}  // namespace sr2ap
