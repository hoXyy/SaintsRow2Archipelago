#include "sr2ap/AtomicFile.hpp"

#include <windows.h>
#include <fstream>

namespace sr2ap {
    bool ReplaceFileAtomically(const std::filesystem::path& path, std::string_view contents) {
        const auto temporary = path.wstring() + L".tmp";
        std::ofstream output(std::filesystem::path(temporary), std::ios::out | std::ios::trunc);
        output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        output.close();

        if (!output) {
            DeleteFileW(temporary.c_str());
            return false;
        }

        if (MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            return true;
        }

        DeleteFileW(temporary.c_str());
        return false;
    }
}  // namespace sr2ap
