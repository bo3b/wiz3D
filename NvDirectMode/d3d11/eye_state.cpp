#include "eye_state.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace NvDirectMode
{

namespace
{
    typedef int  (__cdecl* PFN_GetActiveEye)();
    typedef void (__cdecl* PFN_SetEyeChangeCallback)(void (__cdecl *cb)(int oldEye, int newEye));

    HMODULE                  g_nvapiModule        = nullptr;
    PFN_GetActiveEye         g_pfnGetEye          = nullptr;
    PFN_SetEyeChangeCallback g_pfnSetEyeCallback  = nullptr;
    EyeChangeHandler         g_pendingHandler     = nullptr;
    EyeChangeHandler         g_registeredHandler  = nullptr;

    // Static-storage trampoline: NvApiProxy's callback is plain C-linkage,
    // so we can't directly hand it a member function. This forwards to
    // whatever EyeChangeHandler the d3d11 wrapper code last set.
    void __cdecl EyeChangeTrampoline(int oldEye, int newEye)
    {
        if (g_registeredHandler) g_registeredHandler(oldEye, newEye);
    }

    void Resolve()
    {
#ifdef _WIN64
        HMODULE m = GetModuleHandleA("nvapi64.dll");
#else
        HMODULE m = GetModuleHandleA("nvapi.dll");
#endif
        if (!m)
        {
            if (g_nvapiModule) { g_nvapiModule = nullptr; g_pfnGetEye = nullptr; g_pfnSetEyeCallback = nullptr; }
            return;
        }
        if (m != g_nvapiModule)
        {
            g_pfnGetEye         = (PFN_GetActiveEye)        GetProcAddress(m, "Wiz3D_GetActiveEye");
            g_pfnSetEyeCallback = (PFN_SetEyeChangeCallback)GetProcAddress(m, "Wiz3D_SetEyeChangeCallback");
            g_nvapiModule       = m;
            // If the wrapper already asked to register a handler before
            // nvapi was loaded, do it now.
            if (g_pfnSetEyeCallback && g_pendingHandler)
            {
                g_registeredHandler = g_pendingHandler;
                g_pfnSetEyeCallback(&EyeChangeTrampoline);
                g_pendingHandler = nullptr;
            }
        }
    }
}

int GetActiveEye()
{
    Resolve();
    if (!g_pfnGetEye) return kEyeMono;
    return g_pfnGetEye();
}

void RegisterEyeChangeHandler(EyeChangeHandler handler)
{
    Resolve();
    if (g_pfnSetEyeCallback)
    {
        g_registeredHandler = handler;
        g_pfnSetEyeCallback(handler ? &EyeChangeTrampoline : nullptr);
        g_pendingHandler = nullptr;
    }
    else
    {
        // Defer until next Resolve() succeeds (nvapi loaded later).
        g_pendingHandler = handler;
    }
}

} // namespace NvDirectMode
