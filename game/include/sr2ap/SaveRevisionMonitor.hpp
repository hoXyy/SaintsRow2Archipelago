#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>

namespace sr2ap {
    class SaveRevisionMonitor {
       public:
        using Callback = std::function<void(std::uint32_t)>;
        using LoadStartingCallback = std::function<void(std::uint32_t)>;

        SaveRevisionMonitor();
        ~SaveRevisionMonitor();
        SaveRevisionMonitor(const SaveRevisionMonitor&) = delete;
        SaveRevisionMonitor& operator=(const SaveRevisionMonitor&) = delete;

        bool Install(Callback loaded, Callback saved, LoadStartingCallback loadStarting);
        void Poll();
        void Remove();
        [[nodiscard]] std::optional<std::uint32_t> CurrentChecksum() const;

       private:
        struct Implementation;
        std::unique_ptr<Implementation> implementation_;
    };
}  // namespace sr2ap
