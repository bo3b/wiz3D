#include "eye_state.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace NvDirectMode
{

namespace
{
    typedef int (__cdecl* PFN_GetActiveEye)();

    HMODULE          g_nvapiModule = nullptr;
    PFN_GetActiveEye g_pfn         = nullptr;

    void Resolve()
    {
        // Either nvapi.dll (Win32) or nvapi64.dll (x64) is loaded by the
        // game. Both export Wiz3D_GetActiveEye when our NvApiProxy is the
        // one that got picked up; the real NVIDIA driver doesn't.
#ifdef _WIN64
        HMODULE m = GetModuleHandleA("nvapi64.dll");
#else
        HMODULE m = GetModuleHandleA("nvapi.dll");
#endif
        if (!m)
        {
            if (g_nvapiModule) { g_nvapiModule = nullptr; g_pfn = nullptr; }
            return;
        }
        if (m != g_nvapiModule)
        {
            g_pfn = (PFN_GetActiveEye)GetProcAddress(m, "Wiz3D_GetActiveEye");
            g_nvapiModule = m;
        }
    }
}

int GetActiveEye()
{
    Resolve();
    if (!g_pfn) return kEyeMono;  // bridge unavailable -> degrade to no routing
    return g_pfn();
}

} // namespace NvDirectMode
