#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "util/GameEvents.hpp"

namespace sr2ap {
// Reports Hitman and Chop Shop lists as the game shows them: in the list menu
// that opens after starting a list (or revisiting its start trigger) and in
// the pause menu. Only started lists are reported, as often as they are shown.
class ActivityListMonitor {
   public:
    // Game thread. `list` is one-based.
    using Callback = std::function<void(ActivityList, std::uint32_t list)>;

    ActivityListMonitor();
    ~ActivityListMonitor();
    ActivityListMonitor(const ActivityListMonitor&) = delete;
    ActivityListMonitor& operator=(const ActivityListMonitor&) = delete;

    bool Install(Callback listOpened);
    // Plugin thread, once per loop.
    void Poll();
    void Remove();

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};
}  // namespace sr2ap
