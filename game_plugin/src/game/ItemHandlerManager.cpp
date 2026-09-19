#include "ItemHandlerManager.hpp"

#include <ranges>
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

    if (!dispatcher_.Install()) {
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
    const std::uint32_t threadId) noexcept {
    respectController_.PermitNextSaveRestore(threadId);
}

bool ItemHandlerManager::ResetState() const {
    for (const auto& handler : handlers_) {
        if (!handler->ResetState()) {
            return false;
        }
    }

    return true;
}

void ItemHandlerManager::Update() const {
    for (auto& handler : handlers_) {
        handler->Update();
    }
}

void ItemHandlerManager::Remove() {
    dispatcher_.Remove();
    for (auto & handler : std::views::reverse(handlers_)) {
        handler->Remove();
    }
    handlers_.clear();
    installed_ = false;
}

}  // namespace sr2ap
