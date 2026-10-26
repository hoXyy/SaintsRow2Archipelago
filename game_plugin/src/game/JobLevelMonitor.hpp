#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "util/GameEvents.hpp"

namespace sr2ap {
// Reports each finished job diversion level as the game finishes it. The
// vehicle jobs restart at level 1 every run, so a level can be reported again
// on a later run; nothing done before the hooks were installed is reported.
class JobLevelMonitor {
   public:
    // Game thread. `level` is one-based.
    using Callback = std::function<void(Job, std::uint32_t level)>;

    JobLevelMonitor();
    ~JobLevelMonitor();
    JobLevelMonitor(const JobLevelMonitor&) = delete;
    JobLevelMonitor& operator=(const JobLevelMonitor&) = delete;

    bool Install(Callback levelCompleted);
    // Plugin thread, once per loop.
    void Poll();
    void Remove();

   private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation_;
};
}  // namespace sr2ap
