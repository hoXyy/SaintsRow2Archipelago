#include "sr2ap/ChopShop.hpp"
#include "sr2ap/Addresses.hpp"
#include "sr2ap/Logger.hpp"
#include "sr2ap/Memory.hpp"
#include "sr2ap/ModuleInfo.hpp"

#include <windows.h>
#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>

namespace sr2ap {
    namespace {
        constexpr std::size_t kTargetTagCapacity = 32;
        constexpr std::uint32_t kMaximumLists = 8;
        constexpr std::uint32_t kMaximumVehiclesPerList = 16;

        bool ValidateReaderCode(const ModuleInfo& game) {
            constexpr std::array<std::uint8_t, 12> expected{0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8,
                                                            0x81, 0xEC, 0x24, 0x01, 0x00, 0x00};
            std::array<std::uint8_t, expected.size()> actual{};
            const auto handler = game.base + addresses::kChopShopRowsHandlerRva;
            return SafeCopy(reinterpret_cast<const void*>(handler), actual.data(), actual.size()) && actual == expected;
        }

        bool ReadTargetTag(std::uintptr_t address, std::string& result) {
            const auto value = ReadFixedString(address, kTargetTagCapacity);
            if (!value) {
                return false;
            }
            result = *value;
            if (result.rfind("CHOP_SHOP_TARGET_", 0) != 0 || result.size() <= 17) {
                return false;
            }
            return std::all_of(result.begin(), result.end(),
                               [](unsigned char value) { return std::isalnum(value) || value == '_'; });
        }
    }  // namespace

    ChopShopSnapshot GetChopShopSnapshot() {
        ChopShopSnapshot snapshot;
        const auto game = InspectSupportedGameModule();
        if (!game || !ValidateReaderCode(*game)) {
            snapshot.result = ChopShopReadResult::UnsupportedVersion;
            return snapshot;
        }
        std::uint32_t root{};
        if (!SafeCopy(reinterpret_cast<const void*>(game->base + addresses::kChopShopRootGlobalRva), &root,
                      sizeof(root)) ||
            !root) {
            snapshot.result = ChopShopReadResult::GameNotReady;
            return snapshot;
        }
        std::uint32_t listCount{};
        if (!SafeCopy(reinterpret_cast<const void*>(root + addresses::kChopShopListCountOffset), &listCount,
                      sizeof(listCount)) ||
            listCount == 0 || listCount > kMaximumLists) {
            snapshot.result = ChopShopReadResult::ManagerUnavailable;
            return snapshot;
        }
        for (std::uint32_t list = 0; list < listCount; ++list) {
            const auto descriptor = static_cast<std::uintptr_t>(root) + addresses::kChopShopDescriptorOffset +
                                    list * addresses::kChopShopDescriptorStride;
            std::uint32_t rowBase{};
            if (!SafeCopy(reinterpret_cast<const void*>(descriptor), &rowBase, sizeof(rowBase)) || !rowBase) {
                snapshot.result = ChopShopReadResult::ManagerUnavailable;
                snapshot.vehicles.clear();
                return snapshot;
            }
            std::uint32_t count{};
            if (!SafeCopy(reinterpret_cast<const void*>(rowBase + addresses::kChopShopRowCountOffset), &count,
                          sizeof(count)) ||
                count == 0 || count > kMaximumVehiclesPerList) {
                snapshot.result = ChopShopReadResult::InvalidPointer;
                snapshot.vehicles.clear();
                return snapshot;
            }
            const auto required = static_cast<std::size_t>(count - 1) * addresses::kChopShopRowStride +
                                  addresses::kChopShopRespectOffset + sizeof(std::uint32_t);
            if (!IsReadableAddress(reinterpret_cast<const void*>(rowBase), required) ||
                !IsReadableAddress(reinterpret_cast<const void*>(descriptor + addresses::kChopShopRetrievedFlagsOffset),
                                   count)) {
                snapshot.result = ChopShopReadResult::InvalidPointer;
                snapshot.vehicles.clear();
                return snapshot;
            }
            for (std::uint32_t vehicle = 0; vehicle < count; ++vehicle) {
                const auto row = static_cast<std::uintptr_t>(rowBase) + vehicle * addresses::kChopShopRowStride;
                std::uint8_t storedFlag{};
                std::uint32_t cash{}, respect{};
                std::string tag;
                if (!SafeCopy(
                        reinterpret_cast<const void*>(descriptor + addresses::kChopShopRetrievedFlagsOffset + vehicle),
                        &storedFlag, 1) ||
                    storedFlag > 1 || !ReadTargetTag(row + addresses::kChopShopDossierOffset, tag) ||
                    !SafeCopy(reinterpret_cast<const void*>(row + addresses::kChopShopCashOffset), &cash,
                              sizeof(cash)) ||
                    !SafeCopy(reinterpret_cast<const void*>(row + addresses::kChopShopRespectOffset), &respect,
                              sizeof(respect))) {
                    snapshot.result = ChopShopReadResult::InvalidPointer;
                    snapshot.vehicles.clear();
                    return snapshot;
                }
                snapshot.vehicles.push_back({std::move(tag), list + 1, vehicle + 1, storedFlag == 0, cash, respect});
            }
        }
        snapshot.result = ChopShopReadResult::Success;
        return snapshot;
    }

    void LogChopShopSnapshot(const ChopShopSnapshot& snapshot, bool full) {
        std::size_t retrieved = 0;

        for (const auto& vehicle : snapshot.vehicles) {
            if (vehicle.retrieved) {
                ++retrieved;
            }
        }

        std::uint32_t lists = 0;
        for (const auto& vehicle : snapshot.vehicles) {
            lists = std::max(lists, vehicle.listId);
        }

        LogInfo("ChopShop", std::string("[Manual snapshot] result=") + ToString(snapshot.result) + " lists=" +
                                std::to_string(lists) + " vehicles=" + std::to_string(snapshot.vehicles.size()) +
                                " retrieved=" + std::to_string(retrieved));

        if (full && snapshot.result == ChopShopReadResult::Success) {
            for (const auto& vehicle : snapshot.vehicles) {
                LogInfo("ChopShop", vehicle.targetTag + "=" + (vehicle.retrieved ? "1" : "0") + " cash=" +
                                        std::to_string(vehicle.cash) + " respect=" + std::to_string(vehicle.respect));
            }
        }
    }
}  // namespace sr2ap
