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
#ifdef _WIN64
        HMODULE m = GetModuleHandleA("nvapi64.dll");
#else
        HMODULE m = GetModuleHandleA("nvapi.dll");
#endif
        if (!m) { if (g_nvapiModule) { g_nvapiModule = nullptr; g_pfn = nullptr; } return; }
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
    if (!g_pfn) return kEyeMono;
    return g_pfn();
}

} // namespace NvDirectMode
