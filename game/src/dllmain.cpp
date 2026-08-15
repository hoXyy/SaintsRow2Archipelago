#include <windows.h>
#include "sr2ap/Plugin.hpp"

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);

        if (HANDLE thread = CreateThread(nullptr, 0, sr2ap::PluginThread, module, 0, nullptr)) {
            CloseHandle(thread);
        }

    } else if (reason == DLL_PROCESS_DETACH) {
        sr2ap::RequestShutdown();
    }
    return TRUE;
}
