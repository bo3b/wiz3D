#pragma once

// Diagnostic appender — writes to wiz3D_proxy.log next to the game exe.
// Implemented in AdapterFunctions.cpp; exposed here so other wrapper
// translation units can log along the stereo-setup path (HookDeviceFuncs,
// CreateOutput, etc.).
void DDILog(const char* fmt, ...);

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

