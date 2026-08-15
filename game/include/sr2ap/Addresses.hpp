#pragma once

#include <cstddef>
#include <cstdint>

namespace sr2ap::addresses {
    inline constexpr std::ptrdiff_t kHitmanCategoryHandlerRva = 0x0037E570;
    inline constexpr std::ptrdiff_t kHitmanListTableCandidateRva = 0x00AA1214;
    inline constexpr std::size_t kHitmanListCount = 5;
    inline constexpr std::size_t kHitmanListDescriptorStride = 0x0C;
    inline constexpr std::size_t kHitmanRowStride = 0x3CC;
    inline constexpr std::size_t kHitmanCompletionOffset = 0x74;
    inline constexpr std::size_t kHitmanLocationOffset = 0xEC;
    inline constexpr std::size_t kHitmanRowCountOffset = 0x173C;

    inline constexpr std::ptrdiff_t kChopShopRowsHandlerRva = 0x0037E970;
    inline constexpr std::ptrdiff_t kChopShopRootGlobalRva = 0x0100B178;
    inline constexpr std::size_t kChopShopDescriptorOffset = 0x1288;
    inline constexpr std::size_t kChopShopDescriptorStride = 0x24;
    inline constexpr std::size_t kChopShopListCountOffset = 0x133C;
    inline constexpr std::size_t kChopShopRetrievedFlagsOffset = 0x05;
    inline constexpr std::size_t kChopShopRowCountOffset = 0x398;
    inline constexpr std::size_t kChopShopRowStride = 0x6C;
    inline constexpr std::size_t kChopShopDossierOffset = 0x44;
    inline constexpr std::size_t kChopShopCashOffset = 0x64;
    inline constexpr std::size_t kChopShopRespectOffset = 0x68;

    inline constexpr std::ptrdiff_t kActivityComponentCountRva = 0x01050384;
    inline constexpr std::ptrdiff_t kActivityComponentTableRva = 0x023AC860;
    inline constexpr std::size_t kActivityComponentStride = 0xE8;
    inline constexpr std::size_t kActivityComponentKindOffset = 0x3C;
    inline constexpr std::size_t kActivityComponentObjectOffset = 0x90;
    inline constexpr std::size_t kActivityProgressionKeyOffset = 0x12C;
    inline constexpr std::size_t kActivityInstanceTagOffset = 0x130;
    inline constexpr std::size_t kActivityTotalLevelsOffset = 0x158;
    inline constexpr std::ptrdiff_t kActivityProgressionCountRva = 0x0239719C;
    inline constexpr std::ptrdiff_t kActivityProgressionTableRva = 0x02396E9C;
    inline constexpr std::size_t kActivityProgressionEntryStride = 0x0C;
    inline constexpr std::size_t kActivityProgressionFlagsOffset = 0x08;

    inline constexpr std::ptrdiff_t kCollectibleManagerRva = 0x01D703D4;
    inline constexpr std::size_t kCollectedCdIdsOffset = 0x13C0;
    inline constexpr std::size_t kCollectedCdCountOffset = 0x14B0;
    inline constexpr std::size_t kCollectedCdTargetOffset = 0x14B4;
    inline constexpr std::uint32_t kCollectedCdCapacity = 60;

    inline constexpr std::ptrdiff_t kMissionCompletedQueryRva = 0x002A6E50;

    inline constexpr std::ptrdiff_t kUnlockableProcessorRva = 0x002BBEB0;
    inline constexpr std::ptrdiff_t kUnlockableEnqueueRva = 0x002BBD50;
    inline constexpr std::ptrdiff_t kUnlockableCountRva = 0x0105A29C;
    inline constexpr std::ptrdiff_t kUnlockableArrayPointerOperandRva = 0x002BC809;
    inline constexpr std::ptrdiff_t kUnlockableQueueCountRva = 0x01059A5C;
    inline constexpr std::size_t kUnlockableRecordSize = 0xD0;

    inline constexpr std::ptrdiff_t kCheatActivateRva = 0x002862F0;
    inline constexpr std::ptrdiff_t kGameFrameDispatchSiteRva = 0x00120511;
    inline constexpr std::ptrdiff_t kCheatSaveFlagSiteRva = 0x00287E12;
    inline constexpr std::ptrdiff_t kCheatTableRva = 0x023A6A68;
    inline constexpr std::ptrdiff_t kCheatCountRva = 0x02127B5C;
    inline constexpr std::size_t kCheatRecordSize = 0x68;
    inline constexpr std::size_t kCheatPhoneCodePointerOffset = 0x10;
    inline constexpr std::size_t kCheatActivateCallbackOffset = 0x54;

    inline constexpr std::ptrdiff_t kNotorietySetRva = 0x0015F870;

    inline constexpr std::ptrdiff_t kPlayerGlobalRva = 0x01D703D4;
    inline constexpr std::size_t kPlayerRespectOffset = 0x1200;
    inline constexpr std::ptrdiff_t kRespectPointsPerBarRva = 0x00A98528;
    inline constexpr std::ptrdiff_t kRespectLoadWriteRva = 0x00294124;
    inline constexpr std::ptrdiff_t kRespectAwardWriteRva = 0x005DBB3B;
    inline constexpr std::ptrdiff_t kRespectSetWriteRva = 0x005E1F8C;
    inline constexpr std::ptrdiff_t kRespectAddWriteRva = 0x005E1FF6;

    inline constexpr std::ptrdiff_t kSaveLoadAllRva = 0x00295F00;
    inline constexpr std::ptrdiff_t kSaveLoadInternalCallRva = 0x0029600B;
    inline constexpr std::ptrdiff_t kSaveWriteOpenCallRva = 0x00295240;
    inline constexpr std::ptrdiff_t kCFileOpenRva = 0x007FDC20;

}  // namespace sr2ap::addresses
