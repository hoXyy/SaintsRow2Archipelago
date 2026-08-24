#pragma once

namespace sr2ap {
    enum class ReaderResult {
        Success,
        GameNotReady,
        ManagerUnavailable,
        InvalidPointer,
        InvalidData,
        InvalidFunction,
        UnsupportedVersion,
        ReaderUnavailable
    };

    const char* ToString(ReaderResult value);
}  // namespace sr2ap
