#include <iomanip>
#include <sr2ap/Helpers.hpp>
#include <sstream>

#include "sr2ap/Logger.hpp"

namespace sr2ap {
std::string Hex(std::uintptr_t value, int width) {
    std::ostringstream stream;
    stream << "0x" << std::uppercase << std::hex << std::setw(width)
           << std::setfill('0') << value;
    return stream.str();
}

std::string Narrow(const std::filesystem::path& path) {
    const auto wide = path.wstring();
    if (wide.empty()) {
        return {};
    }

    const auto needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1,
                                            nullptr, 0, nullptr, nullptr);
    if (needed <= 1) {
        return {};
    }

    std::string result(static_cast<std::size_t>(needed), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), -1, result.data(), needed,
                        nullptr, nullptr);
    result.resize(static_cast<std::size_t>(needed - 1));
    return result;
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

bool IsDeliveryReady(const DeliveryContextState state) {
    return state == DeliveryContextState::provisional ||
           state == DeliveryContextState::activeRevision;
}
}  // namespace sr2ap