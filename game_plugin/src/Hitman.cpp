#include "sr2ap/Hitman.hpp"
#include "sr2ap/Addresses.hpp"
#include "sr2ap/AtomicFile.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

#include <windows.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <sstream>

namespace sr2ap {
    namespace {
        constexpr std::size_t kMaxTargetsPerList = 16;
        constexpr std::size_t kLocationCapacity = 32;

        bool ValidateReaderCode(const ModuleInfo& game) {
            constexpr std::array<std::uint8_t, 12> expected{0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8,
                                                            0x81, 0xEC, 0x0C, 0x01, 0x00, 0x00};

            std::array<std::uint8_t, expected.size()> actual{};

            const auto category3 = game.base + addresses::kHitmanCategoryHandlerRva;

            return SafeCopy(reinterpret_cast<const void*>(category3), actual.data(), actual.size()) &&
                   actual == expected;
        }

        bool ReadLocation(std::uintptr_t address, std::string& result) {
            const auto value = ReadFixedString(address, kLocationCapacity);
            if (!value) {
                return false;
            }
            result = *value;
            if (result.rfind("HITMAN_LOC_", 0) != 0 || result.size() <= 11) {
                return false;
            }
            return std::all_of(result.begin(), result.end(),
                               [](unsigned char character) { return std::isalnum(character) || character == '_'; });
        }
    }  // namespace

    HitmanSnapshot GetHitmanSnapshot() {
        HitmanSnapshot snapshot;
        const auto game = InspectSupportedGameModule();

        if (!game) {
            snapshot.result = HitmanReadResult::UnsupportedVersion;
            return snapshot;
        }
        if (!ValidateReaderCode(*game)) {
            snapshot.result = HitmanReadResult::UnsupportedVersion;
            return snapshot;
        }

        const auto table = game->base + addresses::kHitmanListTableCandidateRva;
        for (std::uint32_t listIndex = 0; listIndex < addresses::kHitmanListCount; ++listIndex) {
            std::uint32_t rowBase{};
            const auto descriptor = table + listIndex * addresses::kHitmanListDescriptorStride;
            if (!SafeCopy(reinterpret_cast<const void*>(descriptor), &rowBase, sizeof(rowBase)) || rowBase == 0) {
                snapshot.result = HitmanReadResult::GameNotReady;
                snapshot.targets.clear();
                return snapshot;
            }
            if (!IsInsideModule(game->handle, reinterpret_cast<const void*>(rowBase))) {
                snapshot.result = HitmanReadResult::InvalidPointer;
                snapshot.targets.clear();
                return snapshot;
            }
            std::uint32_t count{};
            if (!SafeCopy(reinterpret_cast<const void*>(rowBase + addresses::kHitmanRowCountOffset), &count,
                          sizeof(count)) ||
                count == 0 || count > kMaxTargetsPerList) {
                snapshot.result = HitmanReadResult::ManagerUnavailable;
                snapshot.targets.clear();
                return snapshot;
            }
            const auto required = static_cast<std::size_t>(count - 1) * addresses::kHitmanRowStride +
                                  addresses::kHitmanLocationOffset + kLocationCapacity;
            if (!IsReadableAddress(reinterpret_cast<const void*>(rowBase), required)) {
                snapshot.result = HitmanReadResult::InvalidPointer;
                snapshot.targets.clear();
                return snapshot;
            }
            for (std::uint32_t targetIndex = 0; targetIndex < count; ++targetIndex) {
                const auto row = static_cast<std::uintptr_t>(rowBase) + targetIndex * addresses::kHitmanRowStride;
                std::uint8_t completion{};
                std::string location;
                if (!SafeCopy(reinterpret_cast<const void*>(row + addresses::kHitmanCompletionOffset), &completion,
                              sizeof(completion)) ||
                    completion > 1 || !ReadLocation(row + addresses::kHitmanLocationOffset, location)) {
                    snapshot.result = HitmanReadResult::InvalidPointer;
                    snapshot.targets.clear();
                    return snapshot;
                }
                snapshot.targets.push_back({std::move(location), listIndex + 1, targetIndex + 1, completion != 0});
            }
        }
        snapshot.result = HitmanReadResult::Success;
        return snapshot;
    }

    void LogHitmanSnapshot(const HitmanSnapshot& snapshot, bool full) {
        std::size_t complete = 0;
        for (const auto& target : snapshot.targets) {
            if (target.complete) {
                ++complete;
            }
        }
        std::ostringstream summary;
        summary << "[Manual snapshot] result=" << ToString(snapshot.result) << " targets=" << snapshot.targets.size()
                << " complete=" << complete;
        LogInfo("Hitman", summary.str());
        if (full && snapshot.result == HitmanReadResult::Success) {
            for (const auto& target : snapshot.targets) {
                LogInfo("Hitman", target.locationTag + "=" + (target.complete ? "1" : "0"));
            }
        }
    }

    bool WriteHitmanStatus(const std::filesystem::path& path, const HitmanSnapshot& snapshot) {
        std::ostringstream output;
        std::size_t complete = 0;
        for (const auto& target : snapshot.targets) {
            if (target.complete) {
                ++complete;
            }
        }
        output << "game_ready=" << (snapshot.result == HitmanReadResult::Success ? 1 : 0) << '\n'
               << "reader_ready=" << (snapshot.result == HitmanReadResult::Success ? 1 : 0) << '\n'
               << "result=" << ToString(snapshot.result) << '\n'
               << "target_count=" << snapshot.targets.size() << '\n'
               << "complete_count=" << complete << "\n\n";
        if (snapshot.result == HitmanReadResult::Success) {
            for (const auto& target : snapshot.targets) {
                output << target.locationTag << '=' << (target.complete ? 1 : 0) << '\n';
            }
        }
        return output && ReplaceFileAtomically(path, output.str());
    }
}  // namespace sr2ap
