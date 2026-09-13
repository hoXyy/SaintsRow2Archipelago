#include "ChopShopReader.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "fmt/format.h"
#include "game/Addresses.hpp"
#include "game/GameState.hpp"
#include "game/Memory.hpp"
#include "progression/BaselineTracker.hpp"
#include "progression/ProgressionReader.hpp"
#include "util/ReaderResult.hpp"

namespace sr2ap {
namespace {
constexpr std::string_view kReaderId{"chop_shop"};

constexpr std::size_t kTargetTagCapacity = 32;
constexpr std::uint32_t kMaximumLists = 8;
constexpr std::uint32_t kMaximumVehiclesPerList = 16;

struct ChopShopVehicleStatus {
    std::string targetTag;
    std::uint32_t listId{};
    std::uint32_t vehicleIndex{};
    bool retrieved{};
};

struct ChopShopSnapshot {
    ReaderResult result{ReaderResult::ReaderUnavailable};
    std::vector<ChopShopVehicleStatus> vehicles;
};

bool ValidateReaderCode(const ModuleInfo& game) {
    constexpr std::array<std::uint8_t, 12> expected{
        0x55, 0x8B, 0xEC, 0x83, 0xE4, 0xF8, 0x81, 0xEC, 0x24, 0x01, 0x00, 0x00};

    return MatchesBytes(game.base + addresses::kChopShopRowsHandlerRva,
                        expected);
}

ChopShopSnapshot ReadSnapshot(const GameContext& context) {
    ChopShopSnapshot snapshot;

    if (!context.IsSupported()) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    const ModuleInfo& game = *context.module;

    if (!ValidateReaderCode(game)) {
        snapshot.result = ReaderResult::UnsupportedVersion;
        return snapshot;
    }

    std::optional<std::uint32_t> root = ReadMemory<std::uint32_t>(
        game.base + addresses::kChopShopRootGlobalRva);

    if (!root || !*root) {
        snapshot.result = ReaderResult::GameNotReady;
        return snapshot;
    }

    std::optional<std::uint32_t> listCount =
        ReadMemory<std::uint32_t>(*root + addresses::kChopShopListCountOffset);

    if (!listCount || *listCount == 0 || *listCount > kMaximumLists) {
        snapshot.result = ReaderResult::ManagerUnavailable;
        return snapshot;
    }

    for (std::uint32_t list = 0; list < *listCount; ++list) {
        const auto descriptor = static_cast<std::uintptr_t>(*root) +
                                addresses::kChopShopDescriptorOffset +
                                list * addresses::kChopShopDescriptorStride;
        std::optional<std::uint32_t> rowBase =
            ReadMemory<std::uint32_t>(descriptor);

        if (!rowBase || !*rowBase) {
            snapshot.result = ReaderResult::ManagerUnavailable;
            snapshot.vehicles.clear();
            return snapshot;
        }

        std::optional<std::uint32_t> count = ReadMemory<std::uint32_t>(
            *rowBase + addresses::kChopShopRowCountOffset);

        if (!count || *count == 0 || *count > kMaximumVehiclesPerList) {
            snapshot.result = ReaderResult::InvalidPointer;
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
            snapshot.result = ReaderResult::InvalidPointer;
            snapshot.vehicles.clear();
            return snapshot;
        }

        for (std::uint32_t vehicle = 0; vehicle < *count; ++vehicle) {
            const auto row = static_cast<std::uintptr_t>(*rowBase) +
                             vehicle * addresses::kChopShopRowStride;

            std::uint8_t storedFlag{};

            auto tag =
                ReadValidatedString(row + addresses::kChopShopDossierOffset,
                                    kTargetTagCapacity, "CHOP_SHOP_TARGET_");
            if (!SafeCopy(
                    reinterpret_cast<const void*>(
                        descriptor + addresses::kChopShopRetrievedFlagsOffset +
                        vehicle),
                    &storedFlag, 1) ||
                storedFlag > 1 || !tag) {
                snapshot.result = ReaderResult::InvalidPointer;
                snapshot.vehicles.clear();
                return snapshot;
            }

            snapshot.vehicles.push_back({
                std::move(*tag),
                list + 1,
                vehicle + 1,
                storedFlag == 0,
            });
        }
    }

    snapshot.result = ReaderResult::Success;
    return snapshot;
}

std::string SerializeStatus(const ChopShopSnapshot& snapshot) {
    if (snapshot.result != ReaderResult::Success) {
        return fmt::format("[{}]\nresult={}\n", kReaderId,
                           ToString(snapshot.result));
    }

    const auto retrieved =
        std::count_if(snapshot.vehicles.begin(), snapshot.vehicles.end(),
                      [](const auto& value) { return value.retrieved; });

    std::string output = fmt::format(
        "[{}]\n"
        "result={}\n"
        "vehicle_count={}\n"
        "retrieved_count={}\n",
        kReaderId, ToString(snapshot.result), snapshot.vehicles.size(),
        retrieved);

    for (const auto& vehicle : snapshot.vehicles) {
        output += fmt::format("{}={}\n", vehicle.targetTag, vehicle.retrieved);
    }

    return output;
}

class ChopShopReader final : public ProgressionReader {
   public:
    std::string_view Id() const noexcept override {
        return kReaderId;
    }

    ReaderUpdate Poll(const GameContext& context) override;

    std::string CaptureStatus(const GameContext& context) const override {
        return SerializeStatus(ReadSnapshot(context));
    }

   private:
    BaselineTracker<std::string, bool> baseline_;
    ReaderResult previousResult_{ReaderResult::ReaderUnavailable};
};

ReaderUpdate ChopShopReader::Poll(const GameContext& context) {
    const auto snapshot = ReadSnapshot(context);
    ReaderUpdate result;
    result.statusSection = SerializeStatus(snapshot);

    if (snapshot.result != ReaderResult::Success) {
        const auto invalidation = baseline_.Invalidate();

        result.changed = invalidation.kind != BaselineUpdateKind::Unchanged ||
                         snapshot.result != previousResult_;

        previousResult_ = snapshot.result;
        return result;
    }

    std::unordered_map<std::string, bool> current;
    current.reserve(snapshot.vehicles.size());

    for (const auto& vehicle : snapshot.vehicles) {
        current.emplace(vehicle.targetTag, vehicle.retrieved);
    }

    const auto update = baseline_.Observe(std::move(current));
    result.changed = update.kind != BaselineUpdateKind::Unchanged;

    if (update.kind == BaselineUpdateKind::Created ||
        update.kind == BaselineUpdateKind::IdentityChanged) {
        for (const auto& vehicle : snapshot.vehicles) {
            if (!vehicle.retrieved) {
                continue;
            }

            result.events.push_back({
                .category = std::string{kReaderId},
                .key = vehicle.targetTag,
                .previous = 0,
                .current = 1,
            });
        }
    }

    for (const auto& change : update.changes) {
        result.events.push_back({
            .category = std::string{kReaderId},
            .key = change.key,
            .previous = change.previous ? 1U : 0U,
            .current = change.current ? 1U : 0U,
        });
    }

    previousResult_ = ReaderResult::Success;

    return result;
}
}  // namespace

ProgressionReaderPtr CreateChopShopReader() {
    return std::make_unique<ChopShopReader>();
}
}  // namespace sr2ap