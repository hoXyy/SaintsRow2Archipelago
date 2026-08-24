#pragma once

#include <filesystem>
#include <string_view>

namespace sr2ap {
    bool ReplaceFileAtomically(const std::filesystem::path& path, std::string_view contents);
}  // namespace sr2ap
