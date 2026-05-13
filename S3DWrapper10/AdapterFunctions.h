#pragma once

// Diagnostic appender — writes to wiz3D_proxy.log next to the game exe.
// Implemented in AdapterFunctions.cpp; exposed here so other wrapper
// translation units can log along the stereo-setup path (HookDeviceFuncs,
// CreateOutput, etc.).
void DDILog(const char* fmt, ...);

// Per-trampoline entry counter for the Draw* DDI hooks. The macro-local
// static gives one counter per call site, so each Draw variant tallies on
// its own. Rate-limited to first 5 + every 10000th so it survives an
// 8-hour gameplay session. Used to prove whether the runtime is actually
// dispatching Draw* calls to our installed function pointers, or whether
// a DDI 11.10 slot mismatch is silently re-routing them.
#define LOG_DRAW_TRAMPOLINE_ENTRY(name, hDeviceArg)                                       \
    do {                                                                                  \
        static int s_count = 0;                                                           \
        ++s_count;                                                                        \
        if (s_count < 5 || (s_count % 10000) == 0)                                        \
            DDILog("DrawEntry[%s][%d]: hDevice.pDrvPrivate=%p\n",                         \
                   (name), s_count, (hDeviceArg).pDrvPrivate);                            \
    } while (0)

// Utility to detect extra device-funcs beyond compiled struct size.
// Implemented inline so consumers in different projects can call it
// without requiring cross-project linking.
static inline bool HasExtraDeviceFuncs(void* pDeviceFuncsBase, size_t compiledSize)
{
    if (!pDeviceFuncsBase) return false;
    BYTE* base = (BYTE*)pDeviceFuncsBase;
    const size_t slotsToScan = 64;
    BYTE* scan = base + compiledSize;
    for (size_t i = 0; i < slotsToScan; i++)
    {
        void* val = NULL;
        __try { val = *(void**)(scan + i * sizeof(void*)); }
        __except (EXCEPTION_EXECUTE_HANDLER) { val = NULL; }

        if (!val) continue;

        HMODULE hMod = NULL;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                              (LPCSTR)val, &hMod))
        {
            DEBUG_MESSAGE(_T("Detected extra device-func pointer @ %p -> module present\n"), val);
            return true;
        }
    }
    return false;
}

