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
#include <psapi.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "psapi.lib")

// ---------------------------------------------------------------------------
// Diagnostic log — shared filename with NvDirectMode/d3d9/d3d10/opengl32 since
// only one of those proxies will load in any single game.
// ---------------------------------------------------------------------------
static FILE* g_logFile = NULL;

static void LogOpen(void)
{
    if (g_logFile) return;
    WCHAR dir[MAX_PATH];
    GetModuleFileNameW(NULL, dir, MAX_PATH);
    WCHAR* pSlash = wcsrchr(dir, L'\\');
    if (pSlash) *(pSlash + 1) = L'\0';
    lstrcatW(dir, L"nvdirectmode_proxy.log");
    g_logFile = _wfopen(dir, L"a");
}

static void Log(const char* fmt, ...)
{
    if (!g_logFile) LogOpen();
    if (!g_logFile) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_logFile, fmt, ap);
    va_end(ap);
    fflush(g_logFile);
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
    if (SUCCEEDED(hr) && ppDevice && *ppDevice)
    {
        NvDirectMode::WrapD3D11DeviceAndContext(ppDevice, ppImmediateContext);
        Log("  wrapped device=%p context=%p\n",
            *ppDevice,
            ppImmediateContext ? *ppImmediateContext : NULL);
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
    Log("D3D11CreateDeviceAndSwapChain(DriverType=%d, Flags=0x%X) called\n", DriverType, Flags);
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
    Log("  real D3D11CreateDeviceAndSwapChain returned 0x%08lX (logical=%ux%u)\n", hr, logicalW, logicalH);
    if (SUCCEEDED(hr) && ppDevice && *ppDevice)
    {
        NvDirectMode::WrapD3D11DeviceAndContext(ppDevice, ppImmediateContext);
        if (logicalW > 0)
            NvDirectMode::SetWrappedDeviceLogicalSize(*ppDevice, logicalW, logicalH);
        if (ppSwapChain && *ppSwapChain)
            *ppSwapChain = NvDirectMode::WrapDXGISwapChain(*ppSwapChain, *ppDevice);
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
