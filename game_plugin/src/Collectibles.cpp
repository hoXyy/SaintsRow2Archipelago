#include "sr2ap/Collectibles.hpp"
#include "sr2ap/Addresses.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

#include <windows.h>
#include <iomanip>
#include <sstream>
#include <unordered_set>

namespace sr2ap {
    namespace {

        std::string HexId(std::uint32_t value) {
            std::ostringstream stream;
            stream << "0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << value;
            return stream.str();
        }
    }  // namespace

    CdSnapshot GetCdSnapshot() {
        CdSnapshot snapshot;
        const auto game = InspectSupportedGameModule();

        if (!game) {
            snapshot.result = CdReadResult::UnsupportedVersion;
            return snapshot;
        }

        std::uint32_t manager{};
        if (!SafeCopy(reinterpret_cast<const void*>(game->base + addresses::kCollectibleManagerRva), &manager,
                      sizeof(manager)) ||
            !manager) {
            snapshot.result = CdReadResult::ManagerUnavailable;
            return snapshot;
        }

        std::uint32_t count{};
        if (!SafeCopy(reinterpret_cast<const void*>(manager + addresses::kCollectedCdCountOffset), &count,
                      sizeof(count)) ||
            !SafeCopy(reinterpret_cast<const void*>(manager + addresses::kCollectedCdTargetOffset), &snapshot.target,
                      sizeof(snapshot.target))) {
            snapshot.result = CdReadResult::InvalidPointer;
            return snapshot;
        }

        if (snapshot.target == 0) {
            snapshot.result = CdReadResult::GameNotReady;
            return snapshot;
        }

        if (snapshot.target != 50 || count > addresses::kCollectedCdCapacity || count > snapshot.target) {
            snapshot.result = CdReadResult::InvalidData;
            return snapshot;
        }

        std::unordered_set<std::uint32_t> identities;
        snapshot.collectedIds.reserve(count);

        for (std::uint32_t index = 0; index < count; ++index) {
            std::uint32_t id{};
            if (!SafeCopy(reinterpret_cast<const void*>(manager + addresses::kCollectedCdIdsOffset + index * 4), &id,
                          sizeof(id))) {
                snapshot.result = CdReadResult::InvalidPointer;
                snapshot.collectedIds.clear();
                return snapshot;
            }

            if (!id || !identities.emplace(id).second) {
                snapshot.result = CdReadResult::InvalidData;
                snapshot.collectedIds.clear();
                return snapshot;
            }

            snapshot.collectedIds.push_back(id);
        }

        snapshot.result = CdReadResult::Success;
        return snapshot;
    }

    void LogCdSnapshot(const CdSnapshot& snapshot, bool full) {
        LogInfo("CDs", "[Manual snapshot] result=" + std::string(ToString(snapshot.result)) + " collected=" +
                           std::to_string(snapshot.collectedIds.size()) + "/" + std::to_string(snapshot.target));
        if (full && snapshot.result == CdReadResult::Success) {
            for (const auto id : snapshot.collectedIds) {
                const auto key = FindCdDistrictKey(id);
                LogInfo("CDs", std::string(key ? key : "unknown") + " id=" + HexId(id) + " complete=1");
            }
        }
    }
}  // namespace sr2ap
