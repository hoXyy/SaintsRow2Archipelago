#include "sr2ap/ReaderResult.hpp"

namespace sr2ap {
    const char* ToString(ReaderResult value) {
        switch (value) {
            case ReaderResult::Success:
                return "success";
            case ReaderResult::GameNotReady:
                return "game_not_ready";
            case ReaderResult::ManagerUnavailable:
                return "manager_unavailable";
            case ReaderResult::InvalidPointer:
                return "invalid_pointer";
            case ReaderResult::InvalidData:
                return "invalid_data";
            case ReaderResult::InvalidFunction:
                return "invalid_function";
            case ReaderResult::UnsupportedVersion:
                return "unsupported_version";
            case ReaderResult::ReaderUnavailable:
                return "reader_unavailable";
        }
        return "unknown";
    }
}  // namespace sr2ap
