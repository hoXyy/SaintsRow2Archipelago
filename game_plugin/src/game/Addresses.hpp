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

// List menu population, just after the list to show is picked: EDI holds the
// Hitman descriptor (may be null), ESI the Chop Shop one. Reached from the
// list menu that opens after starting a list and from the pause menu.
inline constexpr std::ptrdiff_t kHitmanListSelectedRva = 0x0037E5D0;
inline constexpr std::ptrdiff_t kChopShopListSelectedRva = 0x0037E9D3;
// Hitman: nonzero once the list is started. Chop Shop: bit 0.
inline constexpr std::size_t kHitmanListActiveOffset = 0x04;
inline constexpr std::size_t kChopShopListAcceptedOffset = 0x04;

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

inline constexpr std::ptrdiff_t kRacingRecordCountRva = 0x02127B2C;
inline constexpr std::ptrdiff_t kRacingRecordTableRva = 0x023A5EF8;
inline constexpr std::size_t kRacingRecordStride = 0x24;
inline constexpr std::size_t kRacingRecordIdentityHashOffset = 0x04;
inline constexpr std::size_t kRacingRecordMedalOffset = 0x10;
inline constexpr std::size_t kRacingRecordBestTimeOffset = 0x14;
inline constexpr std::size_t kRacingRecordClassOffset = 0x20;

inline constexpr std::ptrdiff_t kCollectibleManagerRva = 0x01D703D4;
inline constexpr std::size_t kCollectedCdIdsOffset = 0x13C0;
inline constexpr std::size_t kCollectedCdCountOffset = 0x14B0;
inline constexpr std::size_t kCollectedCdTargetOffset = 0x14B4;
inline constexpr std::uint32_t kCollectedCdCapacity = 60;
inline constexpr std::size_t kStuntJumpsCompletedOffset = 0x14C0;
inline constexpr std::size_t kStuntJumpsTotalOffset = 0x14C4;

// Save-backed tracked stats, indexed by stat id.
inline constexpr std::ptrdiff_t kTrackedStatTableRva = 0x01052BE0;
inline constexpr std::ptrdiff_t kTrackedStatCountRva = 0x02127CE8;
inline constexpr std::size_t kTrackedStatStride = 0x44;
inline constexpr std::size_t kTrackedStatValueOffset = 0x34;
// "<job> - levels completed": each job raises its stat to the number of
// levels finished.
inline constexpr std::uint32_t kParamedicLevelsStatId = 0x9D;
inline constexpr std::uint32_t kFirefighterLevelsStatId = 0xA0;
inline constexpr std::uint32_t kTaxiLevelsStatId = 0xAF;
inline constexpr std::uint32_t kTowingLevelsStatId = 0xB2;
inline constexpr std::uint32_t kHoingLevelsStatId = 0xC5;
inline constexpr std::uint32_t kJobLevelCount = 10;

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

inline constexpr std::ptrdiff_t kPlayerGlobalRva = 0x01D703D4;
inline constexpr std::size_t kPlayerRespectOffset = 0x1200;
inline constexpr std::size_t kPlayerStyleLevelOffset = 0x1204;
inline constexpr std::size_t kPlayerStylePointsOffset = 0x1208;
inline constexpr std::ptrdiff_t kStyleLevelTableRva = 0x02129760;
inline constexpr std::ptrdiff_t kStyleLevelCountRva = 0x02129764;
inline constexpr std::size_t kStyleLevelEntryStride = 0x08;
inline constexpr std::size_t kStyleLevelMinimumPointsOffset = 0x00;
inline constexpr std::size_t kStyleLevelRespectBonusOffset = 0x04;
// Ten XTable rows follow a zeroed level-zero entry created by the loader.
inline constexpr std::uint32_t kExpectedStyleLevelCount = 11;
// shamefully stolen from Juiced Patch
inline constexpr std::ptrdiff_t kGameLoadedRva = 0x00A94D3E;
inline constexpr std::ptrdiff_t kMenuStateRva = 0x00ABE860;
inline constexpr std::ptrdiff_t kCutsceneActiveRva = 0x02127D14;
inline constexpr std::uint8_t kGameplayBusyState = 2;
inline constexpr std::uint8_t kGameplayMenuState = 3;
inline constexpr std::uint8_t kLoadingMenuState = 13;
inline constexpr std::ptrdiff_t kRespectPointsPerBarRva = 0x00A98528;
inline constexpr std::ptrdiff_t kRespectLoadWriteRva = 0x00294124;
inline constexpr std::ptrdiff_t kRespectAwardWriteRva = 0x005DBB3B;
inline constexpr std::ptrdiff_t kRespectSetWriteRva = 0x005E1F8C;
inline constexpr std::ptrdiff_t kRespectAddWriteRva = 0x005E1FF6;

inline constexpr std::ptrdiff_t kSaveLoadAllRva = 0x00295F00;
inline constexpr std::ptrdiff_t kSaveLoadInternalCallRva = 0x0029600B;
inline constexpr std::ptrdiff_t kSaveWriteOpenCallRva = 0x00295240;
inline constexpr std::ptrdiff_t kCFileOpenRva = 0x007FDC20;

// Hitman and Chop Shop category visibility.
inline constexpr std::ptrdiff_t kChopShopSetAllEnabledRva = 0x00200920;
inline constexpr std::ptrdiff_t kChopShopSetStartTriggersEnabledRva =
    0x00200A60;
inline constexpr std::ptrdiff_t kChopShopSetManagerAvailableRva = 0x00200F10;
inline constexpr std::ptrdiff_t kHitmanSetAllEnabledRva = 0x0023E250;
inline constexpr std::ptrdiff_t kHitmanSetManagerAvailableRva = 0x0023E2D0;
inline constexpr std::ptrdiff_t kChopShopManagerRva = 0x00A9EC20;
inline constexpr std::ptrdiff_t kHitmanManagerRva = 0x00AA11F8;

// Collectible visibility.
inline constexpr std::ptrdiff_t kCdSetAllPickupsEnabledRva = 0x00534060;
inline constexpr std::ptrdiff_t kTagSetAllSpotsEnabledRva = 0x00226B00;

// Stunt jump admission: arms a jump once the player drives through its trigger.
// The gate sits after the prologue saves EBX/ESI/EDI; the exit restores them
// and returns, leaving the jump unarmed.
inline constexpr std::ptrdiff_t kUniqueJumpTryArmRva = 0x0020B4B0;
inline constexpr std::size_t kUniqueJumpTryArmGateOffset = 0x19;
inline constexpr std::size_t kUniqueJumpTryArmExitOffset = 0x212;

// Job diversions. The vehicle jobs have no story gate; each manager's start
// check (vtable +0x14, thiscall, no arguments) returns true once the player
// drives the matching vehicle, and the diversion dispatcher then starts it.
inline constexpr std::ptrdiff_t kParamedicCheckStartRva = 0x001F18D0;
inline constexpr std::ptrdiff_t kFirefighterCheckStartRva = 0x0020BED0;
inline constexpr std::ptrdiff_t kTaxiCheckStartRva = 0x00228EA0;
inline constexpr std::ptrdiff_t kTowingCheckStartRva = 0x0022E7C0;
// Ho-ing starts from location triggers instead: the trigger callback (cdecl,
// returns true to start) and the manager's story-gated trigger toggle
// (vtable +0x4C, thiscall with one byte argument).
inline constexpr std::ptrdiff_t kHoingStartTriggerRva = 0x0023A460;
inline constexpr std::ptrdiff_t kHoingSetTriggersEnabledRva = 0x00238D60;
inline constexpr std::ptrdiff_t kHoingManagerRva = 0x023A4B68;

// Race visibility.
inline constexpr std::ptrdiff_t kRaceToggleAllRva = 0x00280C80;
inline constexpr std::ptrdiff_t kTriggerEnableRva = 0x0066BCE0;

// Native story gate and current gameplay instance.
inline constexpr std::ptrdiff_t kDiversionsAreStoryLockedRva = 0x00203F80;
inline constexpr std::ptrdiff_t kCurrentGameplayInstanceRva = 0x023B3C60;

// Race records.
inline constexpr std::size_t kRacingRecordStartTriggerOffset = 0x18;

// On-screen text (TextRenderer).
inline constexpr std::ptrdiff_t kMenuTextFlushRva = 0x0035B270;
inline constexpr std::ptrdiff_t kStateDispatcherFlushReturnRva = 0x002974E8;
inline constexpr std::ptrdiff_t kGameStateStackRva = 0x0104E760;
inline constexpr std::ptrdiff_t kGameStateTopIndexRva = 0x00A96B7C;
// Prints text with a size scale after x and y.
inline constexpr std::ptrdiff_t kPrintScaledTextRva = 0x00915D00;
inline constexpr std::ptrdiff_t kSetTextColorRva = 0x00914840;
// Body text font handle, filled in at startup (0x0051D290). It's fnt_body_n,
// except in Japanese, Chinese, Korean, Czech, Polish and Russian, where it's
// a scaled copy of that language's font. Scaled copies have the high bit set;
// -1 means loading failed.
inline constexpr std::ptrdiff_t kMenuFontRva = 0x00A98A7C;
// shamefully stolen from Juiced Patch
inline constexpr std::ptrdiff_t kMainMenuFlagRva = 0x02127B75;
inline constexpr std::ptrdiff_t kMainMenuShownRva = 0x00A8D56B;
inline constexpr std::ptrdiff_t kWidescreenFlagRva = 0x021272DD;
inline constexpr std::ptrdiff_t kAspectRatioRva = 0x01EFD8EC;
// Top game state while playing, with the gameplay HUD loaded.
inline constexpr std::uint32_t kGameplayGameState = 5;
// Game state pushed over gameplay for the pause menu, phone and map.
inline constexpr std::uint32_t kInGameMenuGameState = 6;

// Runs a Lua string in the UI (vint) Lua state: code in EAX, state in ESI.
// Same call as Juiced's General::VintExecute.
inline constexpr std::ptrdiff_t kVintLuaExecuteRva = 0x008DA000;
inline constexpr std::ptrdiff_t kVintLuaStateRva = 0x0212A1B8;

// Tags.
inline constexpr std::ptrdiff_t kTagSpotHeadRva = 0x023A3FB4;
inline constexpr std::size_t kTagSpotFlagsOffset = 0x00;
inline constexpr std::size_t kTagSpotMetadataOffset = 0x04;
inline constexpr std::size_t kTagSpotNextOffset = 0x10;
inline constexpr std::size_t kTagIdentityHashOffset = 0x00;
inline constexpr std::size_t kExpectedTagCount = 50;

// Human death (DeathLink).
// Runs once per real death, cdecl with the human as the only stack argument.
// The HP-depleted handler and the bleed-out finisher both call it, and in
// Demolition Derby and races it instantly resets the human instead.
inline constexpr std::ptrdiff_t kHumanDieRva = 0x00560E40;
// bool __cdecl(human*): HP at or below zero, or the bleed-out timer running.
inline constexpr std::ptrdiff_t kHumanIsDeadRva = 0x00585630;
// ECX attacker, EDX damage info, stack human, damage, two ints; the caller
// pops the stack and an x87 value. Negative damage removes all HP. Lua
// character_kill calls it with -1.
inline constexpr std::ptrdiff_t kHumanDamageRva = 0x005847A0;
// __thiscall(human*, const float* hitPoints); ignores invulnerability.
inline constexpr std::ptrdiff_t kHumanSetHitPointsRva = 0x00569C90;
inline constexpr std::size_t kHumanVehicleHandleOffset = 0xD74;
inline constexpr std::size_t kHumanBleedOutTimerOffset = 0xF8C;
inline constexpr std::size_t kHumanHitPointsOffset = 0x1124;

// Object handles: entry (handle & 0xFFF) holds the object pointer first.
inline constexpr std::ptrdiff_t kObjectHandleTableRva = 0x01D49C64;
inline constexpr std::size_t kObjectHandleEntryStride = 0x10;
inline constexpr std::uint32_t kObjectHandleIndexMask = 0xFFF;
inline constexpr std::size_t kObjectHandleOffset = 0x44;
inline constexpr std::size_t kObjectTypeOffset = 0x4C;
inline constexpr std::uint32_t kObjectTypeVehicle = 5;

// Vehicles. The damage function is what Lua vehicle_detonate calls: cdecl,
// 14 arguments.
inline constexpr std::ptrdiff_t kVehicleDamageRva = 0x006AD3D0;
inline constexpr std::size_t kVehiclePositionOffset = 0x14;
// 1 while the vehicle is fully loaded, 0 for a streamed-out "lite" vehicle.
inline constexpr std::size_t kVehicleActiveOffset = 0x84D8;
inline constexpr std::size_t kVehicleHitPointsOffset = 0x8C28;
inline constexpr std::size_t kVehicleStateFlagsOffset = 0x8F50;
inline constexpr std::uint32_t kVehicleWreckedFlag = 0x1;

// Activities. Returns -1 outside activities, otherwise the activity type.
inline constexpr std::ptrdiff_t kCurrentActivityTypeRva = 0x0013C4D0;
inline constexpr std::int32_t kActivityTypeDemolitionDerby = 0;
inline constexpr std::int32_t kActivityTypeRacing = 0x0E;
// Diversions. Returns -1 when no diversion runs, otherwise its type. The
// races you get from the city run as a racing diversion, not the activity,
// but drive the same race manager.
inline constexpr std::ptrdiff_t kCurrentDiversionTypeRva = 0x00203E30;
inline constexpr std::int32_t kDiversionTypeRacing = 0;
// Null outside races (the Racing activity and the racing diversion).
inline constexpr std::ptrdiff_t kRaceManagerRva = 0x02127B14;
inline constexpr std::size_t kRaceManagerStateOffset = 0x00;
inline constexpr std::size_t kRaceManagerPlayerVehicleOffset = 0x17C;
inline constexpr std::uint32_t kRaceStateCountdown = 3;
inline constexpr std::uint32_t kRaceStateRacing = 4;
inline constexpr std::ptrdiff_t kDerbyStateRva = 0x0100A644;
inline constexpr std::ptrdiff_t kDerbyPlayerVehicleRva = 0x02127754;
inline constexpr std::uint32_t kDerbyStateMatch = 7;

// Map reveal. Neighborhood records come from neighborhoods.xtbl; the pause
// map fogs every hood without the explored flag.
inline constexpr std::ptrdiff_t kNeighborhoodCountRva = 0x02126C90;
inline constexpr std::ptrdiff_t kNeighborhoodTableRva = 0x02126C94;
inline constexpr std::size_t kNeighborhoodStride = 0xA8;
inline constexpr std::size_t kNeighborhoodFlagsOffset = 0x58;
inline constexpr std::uint8_t kNeighborhoodExploredFlag = 0x40;
inline constexpr std::uint8_t kNeighborhoodUnexplorableFlag = 0x20;
// Active triggers: circular list, next at +0x188. A trigger with a map icon
// shows the real icon (and is a taxi destination) only once discovered;
// icons created later copy the trigger's discovered flag.
inline constexpr std::ptrdiff_t kActiveTriggerHeadRva = 0x02B8E88C;
inline constexpr std::size_t kTriggerNextOffset = 0x188;
inline constexpr std::size_t kTriggerTypeOffset = 0x13C;
inline constexpr std::size_t kTriggerFlagsOffset = 0x110;
inline constexpr std::size_t kTriggerMapIconTypeOffset = 0x174;
inline constexpr std::size_t kTriggerMapIconOffset = 0x17C;
inline constexpr std::size_t kTriggerMapIconListOffset = 0x180;
inline constexpr std::uint8_t kTriggerDiscoveredFlag = 0x10;
// Unique jumps use the discovered flag to mean completed.
inline constexpr std::uint32_t kTriggerTypeUniqueJump = 0x2F;
// CRC-32 of the trigger name, same hash as CDs and tags.
inline constexpr std::size_t kTriggerIdentityHashOffset = 0xC0;
inline constexpr std::size_t kExpectedStuntJumpCount = 80;
inline constexpr std::int32_t kMapIconListCount = 8;
// Sets the revealed flag on a map icon: list index in EAX, icon in EDI.
inline constexpr std::ptrdiff_t kMapIconRevealRva = 0x003A4DC0;

}  // namespace sr2ap::addresses
