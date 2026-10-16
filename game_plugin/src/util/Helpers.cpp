#include "Helpers.hpp"

#include <sstream>
#include <system_error>

#include "fmt/format.h"
#include "util/Logger.hpp"

namespace sr2ap {
std::string Hex(const std::uintptr_t value, const int width) {
    return fmt::format("0x{:0{}X}", value, width);
}

std::string Narrow(const std::filesystem::path& path) {
    try {
        const auto utf8 = path.u8string();
        return {utf8.begin(), utf8.end()};
    } catch (const std::system_error&) {
        return {};
    }
}

bool Pressed(std::uint32_t key, bool& previous) {
    const bool current =
        key <= 0xFF && (GetAsyncKeyState(static_cast<int>(key)) & 0x8000) != 0;
    const bool edge = current && !previous;
    previous = current;
    return edge;
}

void ReportModule(const char* subsystem, const ModuleInfo& info) {
    LogInfo(subsystem, "path=" + Narrow(info.path));
    LogInfo(subsystem, "base=" + Hex(info.base) +
                           " image_size=" + Hex(info.imageSize) +
                           " pe_timestamp=" + Hex(info.peTimestamp));
}

void ReportAllModules(HMODULE plugin) {
    if (const auto executable = InspectModule(GetModuleHandleW(nullptr))) {
        ReportModule("Module", *executable);
    } else {
        LogError("Module", "Unable to inspect SR2_pc.exe");
    }

    if (const auto self = InspectModule(plugin)) {
        ReportModule("Plugin", *self);
    }
}
}  // namespace sr2ap