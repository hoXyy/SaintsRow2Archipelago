#pragma once
#include <minwindef.h>

#include <cstdint>
#include <filesystem>
#include <string>

#include "sr2ap/ModuleInfo.hpp"

namespace sr2ap {
enum class DeliveryContextState {
    waitingForGameplay,
    provisional,
    awaitingCursor,
    activeRevision,
};

std::string Hex(std::uintptr_t value, int width = 8);
std::string Narrow(const std::filesystem::path& path);
bool Pressed(std::uint32_t key, bool& previous);
void ReportModule(const char* subsystem, const ModuleInfo& info);
void ReportAllModules(HMODULE plugin);
bool IsDeliveryReady(DeliveryContextState state);
}  // namespace sr2ap