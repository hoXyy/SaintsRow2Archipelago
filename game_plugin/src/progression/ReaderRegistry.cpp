#include "ReaderRegistry.hpp"

#include "readers/ActivitiesReader.hpp"
#include "readers/CdReader.hpp"
#include "readers/ChopShopReader.hpp"
#include "readers/HitmanReader.hpp"
#include "readers/MissionReader.hpp"
#include "readers/RacingReader.hpp"
#include "readers/StyleLevelReader.hpp"
#include "readers/TagsReader.hpp"

namespace sr2ap {

ProgressionReaders CreateProgressionReaders() {
    ProgressionReaders readers;
    readers.reserve(8);

    readers.push_back(CreateHitmanReader());
    readers.push_back(CreateChopShopReader());
    readers.push_back(CreateMissionReader());
    readers.push_back(CreateActivitiesReader());
    readers.push_back(CreateRacingReader());
    readers.push_back(CreateCdReader());
    readers.push_back(CreateStyleLevelReader());
    readers.push_back(CreateTagsReader());

    return readers;
}

}  // namespace sr2ap
