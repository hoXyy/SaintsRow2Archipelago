#include "ChopShop.hpp"

#include <windows.h>

#include <array>
#include <optional>

#include "game/Addresses.hpp"
#include "game/GameState.hpp"
#include "game/Memory.hpp"
#include "game/ModuleInfo.hpp"

namespace sr2ap {
namespace {
constexpr std::size_t kTargetTagCapacity = 32;
constexpr std::uint32_t kMaximumLists = 8;
constexpr std::uint32_t kMaximumVehiclesPerList = 16;

bool ValidateReaderCode(const ModuleInfo& game) {
    constexpr std::array<std::uint8_t, 12> expected{
        0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0x24, 0x01, 0x00, 0x00};

    return MatchesBytes(game.base + addresses::kChopShopRowsHandlerRva,
                        expected);
}

}  // namespace

ChopShopSnapshot GetChopShopSnapshot(const GameContext& context) {
    ChopShopSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ChopShopReadResult::UnsupportedVersion;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;

    if (!ValidateReaderCode(game)) {
        snapshot.result = ChopShopReadResult::UnsupportedVersion;
        return snapshot;
    }
    std::optional<std::uint32_t> root = ReadMemory<std::uint32_t>(
        game.base + addresses::kChopShopRootGlobalRva);

    if (!root || !*root) {
        snapshot.result = ChopShopReadResult::GameNotReady;
        return snapshot;
    }

    std::optional<std::uint32_t> listCount =
        ReadMemory<std::uint32_t>(*root + addresses::kChopShopListCountOffset);

    if (!listCount || *listCount == 0 || *listCount > kMaximumLists) {
        snapshot.result = ChopShopReadResult::ManagerUnavailable;
        return snapshot;
    }

    for (std::uint32_t list = 0; list < listCount; ++list) {
        const auto descriptor = static_cast<std::uintptr_t>(*root) +
                                addresses::kChopShopDescriptorOffset +
                                list * addresses::kChopShopDescriptorStride;
        std::optional<std::uint32_t> rowBase =
            ReadMemory<std::uint32_t>(descriptor);

        if (!rowBase || !*rowBase) {
            snapshot.result = ChopShopReadResult::ManagerUnavailable;
            snapshot.vehicles.clear();
            return snapshot;
        }

        std::optional<std::uint32_t> count = ReadMemory<std::uint32_t>(
            *rowBase + addresses::kChopShopRowCountOffset);

        if (!count || *count == 0 || *count > kMaximumVehiclesPerList) {
            snapshot.result = ChopShopReadResult::InvalidPointer;
            snapshot.vehicles.clear();
            return snapshot;
        }

        const auto required = static_cast<std::size_t>(*count - 1) *
                                  addresses::kChopShopRowStride +
                              addresses::kChopShopRespectOffset +
                              sizeof(std::uint32_t);

        if (!IsReadableAddress(reinterpret_cast<const void*>(*rowBase),
                               required) ||
            !IsReadableAddress(
                reinterpret_cast<const void*>(
                    descriptor + addresses::kChopShopRetrievedFlagsOffset),
                *count)) {
            snapshot.result = ChopShopReadResult::InvalidPointer;
            snapshot.vehicles.clear();
            return snapshot;
        }

        for (std::uint32_t vehicle = 0; vehicle < count; ++vehicle) {
            const auto row = static_cast<std::uintptr_t>(*rowBase) +
                             vehicle * addresses::kChopShopRowStride;

            std::uint8_t storedFlag{};
            std::optional<std::uint32_t> cash =
                ReadMemory<std::uint32_t>(row + addresses::kChopShopCashOffset);
            std::optional<std::uint32_t> respect = ReadMemory<std::uint32_t>(
                row + addresses::kChopShopRespectOffset);

            auto tag =
                ReadValidatedString(row + addresses::kChopShopDossierOffset,
                                    kTargetTagCapacity, "CHOP_SHOP_TARGET_");
            if (!SafeCopy(
                    reinterpret_cast<const void*>(
                        descriptor + addresses::kChopShopRetrievedFlagsOffset +
                        vehicle),
                    &storedFlag, 1) ||
                storedFlag > 1 || !tag || !cash || !respect) {
                snapshot.result = ChopShopReadResult::InvalidPointer;
                snapshot.vehicles.clear();
                return snapshot;
            }
            snapshot.vehicles.push_back({std::move(*tag), list + 1, vehicle + 1,
                                         storedFlag == 0, *cash, *respect});
        }
    }
    snapshot.result = ChopShopReadResult::Success;
    return snapshot;
}
}  // namespace sr2ap
