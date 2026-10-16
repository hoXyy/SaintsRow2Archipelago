#include "AtomicFile.hpp"

#include <cstdint>

#include "rust/cxx.h"
#include "sr2ap/src/ffi/fs_ffi.rs.h"

namespace sr2ap {
bool ReplaceFileAtomically(const std::filesystem::path& path,
                           std::string_view contents) {
    const auto& native = path.native();

    try {
        rust::fs_replace_atomically(
            {reinterpret_cast<const std::uint16_t*>(native.data()),
             native.size()},
            {reinterpret_cast<const std::uint8_t*>(contents.data()),
             contents.size()});
        return true;
    } catch (const ::rust::Error&) {
        return false;
    }
}
}  // namespace sr2ap
