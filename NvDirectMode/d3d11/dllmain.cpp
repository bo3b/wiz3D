/* NvDirectMode - d3d11.dll Direct Mode Proxy
 *
 * Stage 1 skeleton: drop this d3d11.dll into a game folder that uses NVIDIA
 * 3D Vision Direct Mode (Tutorial07 sample, modern Direct Mode games). The
 * proxy forwards D3D11CreateDevice / D3D11CreateDeviceAndSwapChain to the
 * real system d3d11.dll unchanged. Wrapping (1b-i), buffer doubling (1b-iii)
 * and per-eye OMSetRenderTargets routing (1b-iv) land in subsequent stages.
 *
 * Like the d3d9 proxy, this TU avoids d3d11.h: the system header declares
 * D3D11CreateDevice without __declspec(dllexport), conflicting with our
 * own extern "C" __declspec(dllexport) definitions. Function-pointer
 * typedefs through void* keep the linkage clean.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../proxy_version.h"
#include "proxy_factory.h"
#include "swapchain_helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <psapi.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")
#include <tlhelp32.h>
#include <psapi.h>

// ---------------------------------------------------------------------------
// Diagnostic log — shared filename with NvDirectMode/d3d9/d3d10/opengl32 since
// only one of those proxies will load in any single game.
// ---------------------------------------------------------------------------
static FILE* g_logFile = NULL;
static int   g_loggingEnabled  = 1;  // overridden by 3DVision_Config.xml LoggingEnabled
static int   g_verboseEnabled  = 1;  // overridden by VerboseLogging — default ON during pre-release
static int   g_swapEyes        = 0;  // overridden by SwapEyes
static int   g_wrapDevices     = 1;  // overridden by WrapDevices
static int   g_outputMode      = 8;  // overridden by OutputMode — see 3DVision_Config.xml comments
                                     //   0/3 = Top-and-Bottom
                                     //   1/2 = Side-by-Side    (default)
                                     //   4   = Line/Row Interleaved
                                     //   5   = Column Interleaved
                                     //   6   = Checkerboard
                                     //   7   = Anaglyph
                                     //   8   = SR weave (Leia / Samsung Odyssey ML displays)
static int   g_anaglyphColour   = 0; // 0=RC (default), 1=GM, 2=AB
static int   g_anaglyphMethod   = 0; // 0=Dubois (default), 1=Compromise, 2=Color, 3=HalfColor, 4=Optimised, 5=Grey, 6=True
static int   g_forceSRWeave     = 0; // diagnostic — bypass SR-incompatible exe blacklist

static void LogOpen(void)
{
    if (g_logFile || !g_loggingEnabled) return;
    WCHAR dir[MAX_PATH];
    GetModuleFileNameW(NULL, dir, MAX_PATH);
    WCHAR* pSlash = wcsrchr(dir, L'\\');
    if (pSlash) *(pSlash + 1) = L'\0';
    lstrcatW(dir, L"nvdirectmode_proxy.log");
    g_logFile = _wfopen(dir, L"a");
}

static void Log(const char* fmt, ...)
{
    if (!g_loggingEnabled) return;
    if (!g_logFile) LogOpen();
    if (!g_logFile) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_logFile, fmt, ap);
    va_end(ap);
    fflush(g_logFile);
}

// Bridge for proxy classes in other TUs (Device11Proxy.cpp, etc.) — log.h
// declares these as extern "C" so they can call into our Log() above.
extern "C" void NvDM_Log(const char* fmt, ...)
{
    if (!g_loggingEnabled) return;
    if (!g_logFile) LogOpen();
    if (!g_logFile) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_logFile, fmt, ap);
    va_end(ap);
    fflush(g_logFile);
}
extern "C" int NvDM_VerboseEnabled() { return g_verboseEnabled; }
extern "C" int NvDM_SwapEyes()       { return g_swapEyes; }
extern "C" int NvDM_OutputMode()     { return g_outputMode; }
// Reduce OutputMode to one of two implemented BB-doubling backends. The
// composite pass on top of the doubled BB then picks the actual final
// output (SBS/TB/interleaved/checkerboard/anaglyph/SR weave) per
// g_outputMode. Everything except the literal T-B modes (0, 3) flattens
// to "doubled width" here so the per-eye capture path lays out a SBS
// shadow regardless of the eventual on-screen format.
extern "C" int NvDM_OutputIsTopBottom() { return (g_outputMode == 0 || g_outputMode == 3) ? 1 : 0; }
extern "C" int NvDM_AnaglyphColour() { return g_anaglyphColour; }
extern "C" int NvDM_AnaglyphMethod() { return g_anaglyphMethod; }
extern "C" int NvDM_ForceSRWeave()   { return g_forceSRWeave; }

// ---------------------------------------------------------------------------
// Tiny config reader — pulls <Tag Value="N"/> ints from 3DVision_Config.xml
// next to the proxy DLL. Skips XML parsing in favour of a minimal substring
// search because we have one schema, three int fields, and no desire to
// link in a real XML lib for a 100-byte config file.
// ---------------------------------------------------------------------------
static int ReadConfigInt(const char* xml, const char* tag, int defaultValue)
{
    char needle[64];
    _snprintf_s(needle, sizeof(needle), _TRUNCATE, "<%s Value=\"", tag);
    const char* p = strstr(xml, needle);
    if (!p) return defaultValue;
    p += strlen(needle);
    return atoi(p);
}

static void LoadConfig(HMODULE hProxy)
{
    WCHAR cfgPath[MAX_PATH];
    GetModuleFileNameW(hProxy, cfgPath, MAX_PATH);
    WCHAR* pSlash = wcsrchr(cfgPath, L'\\');
    if (pSlash) *(pSlash + 1) = L'\0';
    lstrcatW(cfgPath, L"3DVision_Config.xml");

    FILE* f = _wfopen(cfgPath, L"rb");
    if (!f) return;  // no config -> defaults stand
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0 || sz > 16 * 1024) { fclose(f); return; }

    char* buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);

    g_loggingEnabled  = ReadConfigInt(buf, "LoggingEnabled",  g_loggingEnabled);
    g_verboseEnabled  = ReadConfigInt(buf, "VerboseLogging",  g_verboseEnabled);
    g_swapEyes        = ReadConfigInt(buf, "SwapEyes",        g_swapEyes);
    g_wrapDevices     = ReadConfigInt(buf, "WrapDevices",     g_wrapDevices);
    g_outputMode      = ReadConfigInt(buf, "OutputMode",      g_outputMode);
    g_anaglyphColour  = ReadConfigInt(buf, "AnaglyphColour",  g_anaglyphColour);
    g_anaglyphMethod  = ReadConfigInt(buf, "AnaglyphMethod",  g_anaglyphMethod);
    g_forceSRWeave    = ReadConfigInt(buf, "ForceSRWeave",    g_forceSRWeave);

    free(buf);
}

// ---------------------------------------------------------------------------
// VEH crash handler — same filter logic as the d3d9 proxy. Skips PRIV/ILLEGAL
// because third-party DLLs (VMware backdoor probes, CPU feature checks) wrap
// those in __try/__except and a VEH log under loader-lock breaks DllMain init.
// ---------------------------------------------------------------------------
static PVOID g_hVEH = NULL;
static volatile LONG g_crashLogged = 0;

static LONG CALLBACK VectoredCrashHandler(EXCEPTION_POINTERS* pExInfo)
{
    DWORD code = pExInfo->ExceptionRecord->ExceptionCode;

    switch (code)
    {
    case EXCEPTION_ACCESS_VIOLATION:
    case EXCEPTION_STACK_OVERFLOW:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_IN_PAGE_ERROR:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    case EXCEPTION_FLT_INVALID_OPERATION:
        break;
    default:
        return EXCEPTION_CONTINUE_SEARCH;
    }

    if (InterlockedCompareExchange(&g_crashLogged, 1, 0) != 0)
        return EXCEPTION_CONTINUE_SEARCH;

    Log("\n!!! FATAL EXCEPTION (VEH) !!!\n");
    Log("Exception code: 0x%08lX\n", code);
    void* crashAddr = pExInfo->ExceptionRecord->ExceptionAddress;
    Log("Crash address:  %p\n", crashAddr);

    HMODULE hMod = NULL;
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)crashAddr, &hMod))
    {
        WCHAR modName[MAX_PATH];
        GetModuleFileNameW(hMod, modName, MAX_PATH);
        BYTE* base = (BYTE*)hMod;
        DWORD_PTR offset = (BYTE*)crashAddr - base;
        Log("Faulting module: %ls + 0x%IX\n", modName, offset);
    }
    else
    {
        Log("Faulting module: UNKNOWN\n");
    }

    {
        WCHAR dumpPath[MAX_PATH];
        GetModuleFileNameW(NULL, dumpPath, MAX_PATH);
        WCHAR* pSlash = wcsrchr(dumpPath, L'\\');
        if (pSlash) *(pSlash + 1) = L'\0';
        lstrcatW(dumpPath, L"nvdirectmode_crash.dmp");

        HANDLE hFile = CreateFileW(dumpPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            MINIDUMP_EXCEPTION_INFORMATION mei;
            mei.ThreadId = GetCurrentThreadId();
            mei.ExceptionPointers = pExInfo;
            mei.ClientPointers = FALSE;
            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
                              hFile,
                              (MINIDUMP_TYPE)(MiniDumpWithDataSegs |
                                              MiniDumpWithHandleData |
                                              MiniDumpWithThreadInfo |
                                              MiniDumpWithFullMemoryInfo),
                              &mei, NULL, NULL);
            CloseHandle(hFile);
            Log("Minidump written to: %ls\n", dumpPath);
        }
    }

    // Faulting thread metadata — start-address tells us which DLL owns the
    // thread (e.g. SimulatedRealityCore vs the game vs EOSOVH). Critical
    // for diagnosing SR-cohabitation crashes where the fault address is
    // mid-DLL but the *thread* belongs to a known module.
    Log("Faulting thread id: %lu\n", GetCurrentThreadId());

    // Resolve NtQueryInformationThread lazily so we don't need ntdll.lib.
    typedef LONG (WINAPI *pfnNtQIT)(HANDLE, ULONG, PVOID, ULONG, PULONG);
    static pfnNtQIT s_pNtQIT = NULL;
    if (!s_pNtQIT)
    {
        HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
        if (hNtdll)
            s_pNtQIT = (pfnNtQIT)GetProcAddress(hNtdll, "NtQueryInformationThread");
    }
    if (s_pNtQIT)
    {
        // ThreadQuerySetWin32StartAddress = 9
        PVOID startAddr = NULL;
        if (s_pNtQIT(GetCurrentThread(), 9, &startAddr, sizeof(startAddr), NULL) == 0 && startAddr)
        {
            HMODULE hStartMod = NULL;
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                   GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   (LPCSTR)startAddr, &hStartMod))
            {
                WCHAR startModName[MAX_PATH];
                GetModuleFileNameW(hStartMod, startModName, MAX_PATH);
                Log("Faulting thread start: %p (%ls + 0x%IX)\n",
                    startAddr, startModName, (DWORD_PTR)startAddr - (DWORD_PTR)hStartMod);
            }
            else
            {
                Log("Faulting thread start: %p (module UNKNOWN)\n", startAddr);
            }
        }
    }

    // Module list dump — answers "is the SR runtime loaded?" "is EOSOVH
    // loaded?" "are there any other injected DLLs we don't recognise?"
    // We log full paths so callers can spot game-local vs system-wide.
    {
        HMODULE hMods[256];
        DWORD cbNeeded = 0;
        if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded))
        {
            DWORD nMods = cbNeeded / sizeof(HMODULE);
            if (nMods > 256) nMods = 256;
            Log("Loaded modules (%lu):\n", nMods);
            for (DWORD i = 0; i < nMods; ++i)
            {
                WCHAR modPath[MAX_PATH];
                if (GetModuleFileNameW(hMods[i], modPath, MAX_PATH))
                    Log("  %p  %ls\n", hMods[i], modPath);
            }
        }
    }

    // Thread list dump — for each thread in the process, log its TEB-level
    // start address and the module that owns that address. Lets us see which
    // foreign DLLs have spawned worker threads inside the game's process.
    {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (hSnap != INVALID_HANDLE_VALUE)
        {
            THREADENTRY32 te = { sizeof(te) };
            DWORD ownPid = GetCurrentProcessId();
            int   nThreads = 0;
            Log("Threads in process:\n");
            if (Thread32First(hSnap, &te))
            {
                do
                {
                    if (te.th32OwnerProcessID != ownPid) continue;
                    nThreads++;
                    PVOID startAddr = NULL;
                    if (s_pNtQIT)
                    {
                        HANDLE hThr = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, te.th32ThreadID);
                        if (hThr)
                        {
                            s_pNtQIT(hThr, 9, &startAddr, sizeof(startAddr), NULL);
                            CloseHandle(hThr);
                        }
                    }
                    if (startAddr)
                    {
                        HMODULE hStartMod = NULL;
                        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                               (LPCSTR)startAddr, &hStartMod))
                        {
                            WCHAR startModName[MAX_PATH];
                            GetModuleFileNameW(hStartMod, startModName, MAX_PATH);
                            const WCHAR* leaf = wcsrchr(startModName, L'\\');
                            Log("  tid=%lu  start=%p  %ls\n",
                                te.th32ThreadID, startAddr, leaf ? leaf + 1 : startModName);
                        }
                        else
                        {
                            Log("  tid=%lu  start=%p  (module UNKNOWN)\n",
                                te.th32ThreadID, startAddr);
                        }
                    }
                    else
                    {
                        Log("  tid=%lu  start=??\n", te.th32ThreadID);
                    }
                } while (Thread32Next(hSnap, &te));
            }
            CloseHandle(hSnap);
            Log("Thread count: %d\n", nThreads);
        }
    }

    Log("=== CRASH END ===\n");
    if (g_logFile) fflush(g_logFile);
    return EXCEPTION_CONTINUE_SEARCH;
}

// ---------------------------------------------------------------------------
// Real d3d11.dll handles + function pointers (typed as void* / function-ptr
// typedefs to keep d3d11.h out of this TU)
// ---------------------------------------------------------------------------
static HMODULE g_hRealD3D11 = NULL;
static HMODULE g_hProxy     = NULL;

typedef HRESULT (WINAPI *pfnD3D11CreateDevice)(
    void* pAdapter, INT DriverType, HMODULE Software, UINT Flags,
    const void* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion,
    void** ppDevice, void* pFeatureLevel, void** ppImmediateContext);

typedef HRESULT (WINAPI *pfnD3D11CreateDeviceAndSwapChain)(
    void* pAdapter, INT DriverType, HMODULE Software, UINT Flags,
    const void* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion,
    const void* pSwapChainDesc, void** ppSwapChain, void** ppDevice,
    void* pFeatureLevel, void** ppImmediateContext);

static pfnD3D11CreateDevice              g_pfnRealCreateDevice              = NULL;
static pfnD3D11CreateDeviceAndSwapChain  g_pfnRealCreateDeviceAndSwapChain  = NULL;

static BOOL LoadRealD3D11(void)
{
    if (g_hRealD3D11)
        return TRUE;

    WCHAR sysDir[MAX_PATH];
    GetSystemDirectoryW(sysDir, MAX_PATH);
    lstrcatW(sysDir, L"\\d3d11.dll");

    g_hRealD3D11 = LoadLibraryW(sysDir);
    if (!g_hRealD3D11)
    {
        Log("FAIL: Could not load real d3d11.dll from %ls (error %lu)\n", sysDir, GetLastError());
        return FALSE;
    }
    Log("OK: Real d3d11.dll loaded from %ls\n", sysDir);

    g_pfnRealCreateDevice              = (pfnD3D11CreateDevice)             GetProcAddress(g_hRealD3D11, "D3D11CreateDevice");
    g_pfnRealCreateDeviceAndSwapChain  = (pfnD3D11CreateDeviceAndSwapChain) GetProcAddress(g_hRealD3D11, "D3D11CreateDeviceAndSwapChain");

    return TRUE;
}

// ---------------------------------------------------------------------------
// Exported: D3D11CreateDevice — stage 1 passthrough
// ---------------------------------------------------------------------------
extern "C" __declspec(dllexport) HRESULT WINAPI D3D11CreateDevice(
    void* pAdapter, INT DriverType, HMODULE Software, UINT Flags,
    const void* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion,
    void** ppDevice, void* pFeatureLevel, void** ppImmediateContext)
{
    Log("D3D11CreateDevice(DriverType=%d, Flags=0x%X) called\n", DriverType, Flags);
    if (!LoadRealD3D11() || !g_pfnRealCreateDevice) return E_FAIL;
    HRESULT hr = g_pfnRealCreateDevice(pAdapter, DriverType, Software, Flags,
                                       pFeatureLevels, FeatureLevels, SDKVersion,
                                       ppDevice, pFeatureLevel, ppImmediateContext);
    Log("  real D3D11CreateDevice returned 0x%08lX\n", hr);
    if (SUCCEEDED(hr) && ppDevice && *ppDevice && g_wrapDevices)
    {
        NvDirectMode::WrapD3D11DeviceAndContext(ppDevice, ppImmediateContext);
        Log("  wrapped (no-swap): device=%p context=%p\n",
            *ppDevice,
            ppImmediateContext ? *ppImmediateContext : NULL);
    }
    else if (!g_wrapDevices)
    {
        Log("  passthrough (no-swap): WrapDevices=0; device=%p context=%p left unwrapped\n",
            (ppDevice ? *ppDevice : NULL),
            (ppImmediateContext ? *ppImmediateContext : NULL));
    }
    else
    {
        Log("  no wrap (no-swap): hr=0x%08lX device=%p (probe / device-less call)\n",
            hr, (ppDevice ? *ppDevice : NULL));
    }
    return hr;
}

// ---------------------------------------------------------------------------
// Exported: D3D11CreateDeviceAndSwapChain — stage 1 passthrough
// ---------------------------------------------------------------------------
extern "C" __declspec(dllexport) HRESULT WINAPI D3D11CreateDeviceAndSwapChain(
    void* pAdapter, INT DriverType, HMODULE Software, UINT Flags,
    const void* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion,
    const void* pSwapChainDesc, void** ppSwapChain, void** ppDevice,
    void* pFeatureLevel, void** ppImmediateContext)
{
    Log("D3D11CreateDeviceAndSwapChain(DriverType=%d, Flags=0x%X, ppDevice=%p, ppSwapChain=%p) called\n",
        DriverType, Flags, (void*)ppDevice, (void*)ppSwapChain);
    if (!LoadRealD3D11() || !g_pfnRealCreateDeviceAndSwapChain) return E_FAIL;

    // 1b-iii: substitute a doubled-width copy of the game's swap-chain
    // desc. The helper hides DXGI_SWAP_CHAIN_DESC behind a void* so this
    // TU stays free of dxgi.h / d3d11.h.
    unsigned int logicalW = 0, logicalH = 0;
    const void* pDescForReal = pSwapChainDesc;
    if (pSwapChainDesc)
        pDescForReal = NvDirectMode::MakeDoubledSwapChainDesc(pSwapChainDesc, &logicalW, &logicalH);

    HRESULT hr = g_pfnRealCreateDeviceAndSwapChain(pAdapter, DriverType, Software, Flags,
                                                   pFeatureLevels, FeatureLevels, SDKVersion,
                                                   pDescForReal, ppSwapChain, ppDevice,
                                                   pFeatureLevel, ppImmediateContext);
    // Probe-vs-real swap chains can be told apart by size: anything below
    // ~16x16 is almost certainly a feature-detection probe; doubling it
    // doesn't matter because the game won't render to it.
    const char* kind = (logicalW <= 16 || logicalH <= 16) ? " (likely probe)" : "";
    Log("  real D3D11CreateDeviceAndSwapChain returned 0x%08lX (logical=%ux%u)%s\n",
        hr, logicalW, logicalH, kind);
    if (SUCCEEDED(hr) && ppDevice && *ppDevice && g_wrapDevices)
    {
        NvDirectMode::WrapD3D11DeviceAndContext(ppDevice, ppImmediateContext);
        if (logicalW > 0)
            NvDirectMode::SetWrappedDeviceLogicalSize(*ppDevice, logicalW, logicalH);
        if (ppSwapChain && *ppSwapChain)
            *ppSwapChain = NvDirectMode::WrapDXGISwapChain(*ppSwapChain, *ppDevice);
        Log("  wrapped (and-swap): device=%p context=%p swapChain=%p\n",
            *ppDevice,
            ppImmediateContext ? *ppImmediateContext : NULL,
            ppSwapChain ? *ppSwapChain : NULL);
    }
    else if (!g_wrapDevices)
    {
        Log("  passthrough (and-swap): WrapDevices=0; device=%p ctx=%p sc=%p left unwrapped\n",
            (ppDevice ? *ppDevice : NULL),
            (ppImmediateContext ? *ppImmediateContext : NULL),
            (ppSwapChain ? *ppSwapChain : NULL));
    }
    else
    {
        Log("  no wrap: hr=0x%08lX device=%p (probe / device-less call)\n",
            hr, (ppDevice ? *ppDevice : NULL));
    }
    return hr;
}

// ---------------------------------------------------------------------------
// DllMain
// ---------------------------------------------------------------------------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        g_hProxy = hModule;
        DisableThreadLibraryCalls(hModule);
        LoadConfig(hModule);   // sets g_loggingEnabled / g_verboseEnabled / g_swapEyes
        LogOpen();
        g_hVEH = AddVectoredExceptionHandler(1, VectoredCrashHandler);
        {
            WCHAR exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            Log("=== NvDirectMode " DISPLAYED_VERSION " - d3d11 proxy loaded ===\n");
            Log("Game exe: %ls\n", exePath);
            WCHAR proxyPath[MAX_PATH];
            GetModuleFileNameW(hModule, proxyPath, MAX_PATH);
            Log("Proxy DLL: %ls\n", proxyPath);
            Log("Config:    OutputMode=%d (%s)  WrapDevices=%d  SwapEyes=%d  LoggingEnabled=%d  VerboseLogging=%d  ForceSRWeave=%d\n",
                g_outputMode, NvDM_OutputIsTopBottom() ? "Top-and-Bottom" : "Side-by-Side",
                g_wrapDevices, g_swapEyes, g_loggingEnabled, g_verboseEnabled, g_forceSRWeave);
        }
        break;

    case DLL_PROCESS_DETACH:
        Log("=== NvDirectMode d3d11 proxy unloading ===\n");
        if (g_hVEH)
        {
            RemoveVectoredExceptionHandler(g_hVEH);
            g_hVEH = NULL;
        }
        if (g_hRealD3D11)
        {
            FreeLibrary(g_hRealD3D11);
            g_hRealD3D11 = NULL;
        }
        if (g_logFile)
        {
            fclose(g_logFile);
            g_logFile = NULL;
        }
        break;
    }
    return TRUE;
}
