#pragma once
#include <windows.h>

namespace sr2ap {
    DWORD WINAPI PluginThread(void* parameter);
    void RequestShutdown();
}  // namespace sr2ap
