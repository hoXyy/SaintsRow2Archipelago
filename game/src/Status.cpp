#include "sr2ap/Status.hpp"
#include "sr2ap/AtomicFile.hpp"

namespace sr2ap {
    bool WriteProgressionStatus(const std::filesystem::path& path, const ProgressionSnapshot& snapshot) {
        return ReplaceFileAtomically(path, SerializeProgressionStatus(snapshot));
    }

    bool WriteProgressionStatus(const std::filesystem::path& path,
                                const HitmanSnapshot& hitman,
                                const ChopShopSnapshot& chopShop,
                                const MissionSnapshot& missions,
                                const ActivitySnapshot& activities,
                                const CdSnapshot& cds) {
        return ReplaceFileAtomically(path, SerializeProgressionStatus(hitman, chopShop, missions, activities, cds));
    }
}  // namespace sr2ap
