#pragma once

#include <windows.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sr2ap {
struct ModuleSection {
    std::string name;
    std::uintptr_t address{};
    std::size_t size{};
    std::uint32_t characteristics{};
};

struct ModuleInfo {
    HMODULE handle{};
    std::filesystem::path path;
    std::uintptr_t base{};
    std::uint32_t imageSize{};
    std::uint32_t peTimestamp{};
    std::vector<ModuleSection> sections;
};

std::optional<ModuleInfo> InspectModule(HMODULE module);
// Inspected once and cached for the life of the process; null when the
// running executable is not the supported SR2 build.
const ModuleInfo* InspectSupportedGameModule();
std::optional<ModuleInfo> FindLoadedModule(const wchar_t* name);
bool IsSupportedExecutable(const ModuleInfo& module);
}  // namespace sr2ap
