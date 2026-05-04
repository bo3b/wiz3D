// DxgiVendorProxy.cpp
// Proxy dxgi.dll that spoofs AMD GPU vendor ID (0x1002) in DXGI adapter
// enumeration, so AMD HD3D-aware games call LoadLibrary("atidxx32.dll")
// on non-AMD hardware.
//
// *** NOT NEEDED — DO NOT SHIP dxgi.dll TO END USERS ***
//
// D3d11VendorProxy (d3d11.dll) supersedes this entirely:
//   - It performs the same GetDesc / GetDesc1 / EnumAdapters vtable hooks.
//   - It uses MinHook to byte-patch CreateDXGIFactory* directly in the
//     already-loaded system dxgi.dll, so hooks fire regardless of DLL load
//     order or Steam overlay pre-loading system dxgi.dll.
//   - A local dxgi.dll proxy is silently skipped on every Steam game because
//     Steam's GameOverlayRenderer pre-loads system32\dxgi.dll before the
//     game's import table resolves, preventing the local proxy from loading.
//   - D3d11VendorProxy is a strict superset: it also hooks IsWindowedStereoEnabled,
//     CreateSwapChainForHwnd, Present, and installs NvAPI blocking / registry
//     spoofing — none of which are present here.
//
// This project is kept in the solution for reference only. If you want users
// to be able to drop ReShade (or any other dxgi.dll tool) into the game folder
// alongside HD3D, simply omit our dxgi.dll from the release package — d3d11.dll
// alone is sufficient.
//
// d3d11.dll (Win10 x86 + x64) imports only CreateDXGIFactory2 from dxgi.dll.
// Games also call CreateDXGIFactory / CreateDXGIFactory1 directly.
// All three are intercepted here; the factory's EnumAdapters vtable[7/12] and
// the adapter's GetDesc vtable[8] / GetDesc1 vtable[10] are then hooked via
// MinHook so every enumeration returns VendorId == 0x1002.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dxgi.h>
#include "MinHook.h"

static HMODULE g_hReal = nullptr;

// ---- Diagnostic log --------------------------------------------------------

static void WriteLog(const char* msg)
{
    wchar_t path[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, path)) return;
    wcscat_s(path, MAX_PATH, L"HD3D_dxgi.log");
    HANDLE h = CreateFileW(path, FILE_APPEND_DATA,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(h, msg, static_cast<DWORD>(strlen(msg)), &written, nullptr);
    CloseHandle(h);
}

// ---- Real function lookup --------------------------------------------------

static FARPROC RealFn(const char* name)
{
    return g_hReal ? GetProcAddress(g_hReal, name) : nullptr;
}

// ---- Hook function types ---------------------------------------------------

typedef HRESULT (WINAPI* PFN_GetDesc)(IDXGIAdapter*, DXGI_ADAPTER_DESC*);
typedef HRESULT (WINAPI* PFN_GetDesc1)(IDXGIAdapter1*, DXGI_ADAPTER_DESC1*);
typedef HRESULT (WINAPI* PFN_EnumAdapters)(IDXGIFactory*, UINT, IDXGIAdapter**);
typedef HRESULT (WINAPI* PFN_EnumAdapters1)(IDXGIFactory1*, UINT, IDXGIAdapter1**);

static PFN_GetDesc       g_pfnGetDesc       = nullptr;
static PFN_GetDesc1      g_pfnGetDesc1      = nullptr;
static PFN_EnumAdapters  g_pfnEnumAdapters  = nullptr;
static PFN_EnumAdapters1 g_pfnEnumAdapters1 = nullptr;

static bool g_bGetDescHooked       = false;
static bool g_bGetDesc1Hooked      = false;
static bool g_bEnumAdaptersHooked  = false;
static bool g_bEnumAdapters1Hooked = false;

// ---- Hook implementations -------------------------------------------------

static HRESULT WINAPI HookedGetDesc(IDXGIAdapter* pAdapter, DXGI_ADAPTER_DESC* pDesc)
{
    HRESULT hr = g_pfnGetDesc(pAdapter, pDesc);
    if (SUCCEEDED(hr) && pDesc)
    {
        char buf[80];
        wsprintfA(buf, "[DxgiProxy] GetDesc: VendorId 0x%04X->0x1002\n", pDesc->VendorId);
        WriteLog(buf);
        pDesc->VendorId = 0x1002;
    }
    return hr;
}

static HRESULT WINAPI HookedGetDesc1(IDXGIAdapter1* pAdapter, DXGI_ADAPTER_DESC1* pDesc)
{
    HRESULT hr = g_pfnGetDesc1(pAdapter, pDesc);
    if (SUCCEEDED(hr) && pDesc)
    {
        char buf[80];
        wsprintfA(buf, "[DxgiProxy] GetDesc1: VendorId 0x%04X->0x1002\n", pDesc->VendorId);
        WriteLog(buf);
        pDesc->VendorId = 0x1002;
    }
    return hr;
}

// Forward declaration
static void HookAdapter(IDXGIAdapter* pA);

static HRESULT WINAPI HookedEnumAdapters(IDXGIFactory* pFactory, UINT Adapter, IDXGIAdapter** ppAdapter)
{
    HRESULT hr = g_pfnEnumAdapters(pFactory, Adapter, ppAdapter);
    if (SUCCEEDED(hr) && ppAdapter && *ppAdapter)
        HookAdapter(*ppAdapter);
    return hr;
}

static HRESULT WINAPI HookedEnumAdapters1(IDXGIFactory1* pFactory, UINT Adapter, IDXGIAdapter1** ppAdapter)
{
    HRESULT hr = g_pfnEnumAdapters1(pFactory, Adapter, ppAdapter);
    if (SUCCEEDED(hr) && ppAdapter && *ppAdapter)
        HookAdapter(static_cast<IDXGIAdapter*>(*ppAdapter));
    return hr;
}

// ---- Hook installation ----------------------------------------------------

// vtable layout (COM inheritance):
//   IDXGIAdapter:  QI[0] AddRef[1] Release[2]  SetPrivateData[3] SetPrivateDataInterface[4]
//                  GetPrivateData[5] GetParent[6]  EnumOutputs[7] GetDesc[8] CheckInterfaceSupport[9]
//   IDXGIAdapter1: ...GetDesc1[10]
//   IDXGIFactory:  QI[0] AddRef[1] Release[2]  SetPrivateData[3] SetPrivateDataInterface[4]
//                  GetPrivateData[5] GetParent[6]  EnumAdapters[7] MakeWindowAssociation[8]
//                  GetWindowAssociation[9] CreateSwapChain[10] CreateSoftwareAdapter[11]
//   IDXGIFactory1: ...EnumAdapters1[12] IsCurrent[13]

static void HookAdapter(IDXGIAdapter* pA)
{
    void** vt = *reinterpret_cast<void***>(pA);

    if (!g_bGetDescHooked)
    {
        if (MH_CreateHook(vt[8], &HookedGetDesc,
                reinterpret_cast<void**>(&g_pfnGetDesc)) == MH_OK)
        {
            MH_EnableHook(vt[8]);
            g_bGetDescHooked = true;
            WriteLog("[DxgiProxy] IDXGIAdapter::GetDesc hooked\n");
        }
    }

    IDXGIAdapter1* pA1 = nullptr;
    if (!g_bGetDesc1Hooked &&
        SUCCEEDED(pA->QueryInterface(__uuidof(IDXGIAdapter1),
                reinterpret_cast<void**>(&pA1))))
    {
        void** vt1 = *reinterpret_cast<void***>(pA1);
        if (MH_CreateHook(vt1[10], &HookedGetDesc1,
                reinterpret_cast<void**>(&g_pfnGetDesc1)) == MH_OK)
        {
            MH_EnableHook(vt1[10]);
            g_bGetDesc1Hooked = true;
            WriteLog("[DxgiProxy] IDXGIAdapter1::GetDesc1 hooked\n");
        }
        pA1->Release();
    }
}

static void HookFactory(void* pRaw)
{
    if (!pRaw) return;

    IDXGIFactory* pF = nullptr;
    if (FAILED(static_cast<IUnknown*>(pRaw)->QueryInterface(
            __uuidof(IDXGIFactory), reinterpret_cast<void**>(&pF))))
    {
        WriteLog("[DxgiProxy] HookFactory: QI(IDXGIFactory) failed\n");
        return;
    }
    WriteLog("[DxgiProxy] HookFactory: installing hooks\n");

    // Pre-hook adapter 0 before the EnumAdapters hook is active
    IDXGIAdapter* pA0 = nullptr;
    if (SUCCEEDED(pF->EnumAdapters(0, &pA0)))
    {
        HookAdapter(pA0);
        pA0->Release();
    }

    void** vt = *reinterpret_cast<void***>(pF);
    if (!g_bEnumAdaptersHooked)
    {
        if (MH_CreateHook(vt[7], &HookedEnumAdapters,
                reinterpret_cast<void**>(&g_pfnEnumAdapters)) == MH_OK)
        {
            MH_EnableHook(vt[7]);
            g_bEnumAdaptersHooked = true;
            WriteLog("[DxgiProxy] IDXGIFactory::EnumAdapters hooked\n");
        }
    }

    IDXGIFactory1* pF1 = nullptr;
    if (!g_bEnumAdapters1Hooked &&
        SUCCEEDED(pF->QueryInterface(__uuidof(IDXGIFactory1),
                reinterpret_cast<void**>(&pF1))))
    {
        void** vt1 = *reinterpret_cast<void***>(pF1);
        if (MH_CreateHook(vt1[12], &HookedEnumAdapters1,
                reinterpret_cast<void**>(&g_pfnEnumAdapters1)) == MH_OK)
        {
            MH_EnableHook(vt1[12]);
            g_bEnumAdapters1Hooked = true;
            WriteLog("[DxgiProxy] IDXGIFactory1::EnumAdapters1 hooked\n");
        }
        pF1->Release();
    }

    pF->Release();
}

// ---- DXGI factory interceptors --------------------------------------------

typedef HRESULT (WINAPI* PFN_CreateDXGIFactory)(REFIID, void**);
typedef HRESULT (WINAPI* PFN_CreateDXGIFactory1)(REFIID, void**);
typedef HRESULT (WINAPI* PFN_CreateDXGIFactory2)(UINT, REFIID, void**);

extern "C" HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** ppFactory)
{
    WriteLog("[DxgiProxy] CreateDXGIFactory\n");
    static auto pfn = reinterpret_cast<PFN_CreateDXGIFactory>(RealFn("CreateDXGIFactory"));
    if (!pfn) return E_NOTIMPL;
    HRESULT hr = pfn(riid, ppFactory);
    if (SUCCEEDED(hr) && ppFactory && *ppFactory)
        HookFactory(*ppFactory);
    return hr;
}

extern "C" HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** ppFactory)
{
    WriteLog("[DxgiProxy] CreateDXGIFactory1\n");
    static auto pfn = reinterpret_cast<PFN_CreateDXGIFactory1>(RealFn("CreateDXGIFactory1"));
    if (!pfn) return E_NOTIMPL;
    HRESULT hr = pfn(riid, ppFactory);
    if (SUCCEEDED(hr) && ppFactory && *ppFactory)
        HookFactory(*ppFactory);
    return hr;
}

extern "C" HRESULT WINAPI CreateDXGIFactory2(UINT Flags, REFIID riid, void** ppFactory)
{
    WriteLog("[DxgiProxy] CreateDXGIFactory2\n");
    static auto pfn = reinterpret_cast<PFN_CreateDXGIFactory2>(RealFn("CreateDXGIFactory2"));
    if (!pfn) return E_NOTIMPL;
    HRESULT hr = pfn(Flags, riid, ppFactory);
    if (SUCCEEDED(hr) && ppFactory && *ppFactory)
        HookFactory(*ppFactory);
    return hr;
}

// ---- Passthrough thunks ---------------------------------------------------
// d3d11.dll (Win10 x86 + x64) imports only CreateDXGIFactory2.
// All other exports below are needed for completeness.

typedef HRESULT (WINAPI* PFN_DXGID3D10CreateDevice)(HMODULE, void*, void*, UINT, const void*, void**);
extern "C" HRESULT WINAPI DXGID3D10CreateDevice(HMODULE h, void* pF, void* pA, UINT fl, const void* pu, void** pp)
{
    static auto pfn = reinterpret_cast<PFN_DXGID3D10CreateDevice>(RealFn("DXGID3D10CreateDevice"));
    return pfn ? pfn(h, pF, pA, fl, pu, pp) : E_NOTIMPL;
}

typedef HRESULT (WINAPI* PFN_DXGID3D10CreateLayeredDevice)(const void*, UINT, const void*, REFIID, void**);
extern "C" HRESULT WINAPI DXGID3D10CreateLayeredDevice(const void* p1, UINT n, const void* p3, REFIID r, void** pp)
{
    static auto pfn = reinterpret_cast<PFN_DXGID3D10CreateLayeredDevice>(RealFn("DXGID3D10CreateLayeredDevice"));
    return pfn ? pfn(p1, n, p3, r, pp) : E_NOTIMPL;
}

typedef SIZE_T (WINAPI* PFN_DXGID3D10GetLayeredDeviceSize)(const void*, UINT);
extern "C" SIZE_T WINAPI DXGID3D10GetLayeredDeviceSize(const void* p, UINT n)
{
    static auto pfn = reinterpret_cast<PFN_DXGID3D10GetLayeredDeviceSize>(RealFn("DXGID3D10GetLayeredDeviceSize"));
    return pfn ? pfn(p, n) : 0;
}

typedef HRESULT (WINAPI* PFN_DXGID3D10RegisterLayers)(const void*, UINT);
extern "C" HRESULT WINAPI DXGID3D10RegisterLayers(const void* p, UINT n)
{
    static auto pfn = reinterpret_cast<PFN_DXGID3D10RegisterLayers>(RealFn("DXGID3D10RegisterLayers"));
    return pfn ? pfn(p, n) : E_NOTIMPL;
}

typedef HRESULT (WINAPI* PFN_DXGIGetDebugInterface1)(UINT, REFIID, void**);
extern "C" HRESULT WINAPI DXGIGetDebugInterface1(UINT Flags, REFIID riid, void** ppDebug)
{
    static auto pfn = reinterpret_cast<PFN_DXGIGetDebugInterface1>(RealFn("DXGIGetDebugInterface1"));
    return pfn ? pfn(Flags, riid, ppDebug) : E_NOTIMPL;
}

typedef HRESULT (WINAPI* PFN_DXGIDeclareAdapterRemovalSupport)();
extern "C" HRESULT WINAPI DXGIDeclareAdapterRemovalSupport()
{
    static auto pfn = reinterpret_cast<PFN_DXGIDeclareAdapterRemovalSupport>(RealFn("DXGIDeclareAdapterRemovalSupport"));
    return pfn ? pfn() : S_OK;
}

typedef void (WINAPI* PFN_DXGIDisableVBlankVirtualization)();
extern "C" void WINAPI DXGIDisableVBlankVirtualization()
{
    static auto pfn = reinterpret_cast<PFN_DXGIDisableVBlankVirtualization>(RealFn("DXGIDisableVBlankVirtualization"));
    if (pfn) pfn();
}

typedef void (WINAPI* PFN_DXGIDumpJournal)(void*);
extern "C" void WINAPI DXGIDumpJournal(void* p)
{
    static auto pfn = reinterpret_cast<PFN_DXGIDumpJournal>(RealFn("DXGIDumpJournal"));
    if (pfn) pfn(p);
}

typedef HRESULT (WINAPI* PFN_DXGIReportAdapterConfiguration)(UINT);
extern "C" HRESULT WINAPI DXGIReportAdapterConfiguration(UINT flags)
{
    static auto pfn = reinterpret_cast<PFN_DXGIReportAdapterConfiguration>(RealFn("DXGIReportAdapterConfiguration"));
    return pfn ? pfn(flags) : E_NOTIMPL;
}

typedef HRESULT (WINAPI* PFN_ApplyCompatResolutionQuirking)(UINT, BOOL*);
extern "C" HRESULT WINAPI ApplyCompatResolutionQuirking(UINT ordinal, BOOL* enabled)
{
    static auto pfn = reinterpret_cast<PFN_ApplyCompatResolutionQuirking>(RealFn("ApplyCompatResolutionQuirking"));
    return pfn ? pfn(ordinal, enabled) : S_OK;
}

typedef HRESULT (WINAPI* PFN_CompatString)(void*, void*);
extern "C" HRESULT WINAPI CompatString(void* p1, void* p2)
{
    static auto pfn = reinterpret_cast<PFN_CompatString>(RealFn("CompatString"));
    return pfn ? pfn(p1, p2) : E_NOTIMPL;
}

typedef HRESULT (WINAPI* PFN_CompatValue)(void*, void*);
extern "C" HRESULT WINAPI CompatValue(void* p1, void* p2)
{
    static auto pfn = reinterpret_cast<PFN_CompatValue>(RealFn("CompatValue"));
    return pfn ? pfn(p1, p2) : E_NOTIMPL;
}

typedef HRESULT (WINAPI* PFN_PIXBeginCapture)(DWORD, const void*);
extern "C" HRESULT WINAPI PIXBeginCapture(DWORD flags, const void* params)
{
    static auto pfn = reinterpret_cast<PFN_PIXBeginCapture>(RealFn("PIXBeginCapture"));
    return pfn ? pfn(flags, params) : E_NOTIMPL;
}

typedef HRESULT (WINAPI* PFN_PIXEndCapture)(BOOL);
extern "C" HRESULT WINAPI PIXEndCapture(BOOL force)
{
    static auto pfn = reinterpret_cast<PFN_PIXEndCapture>(RealFn("PIXEndCapture"));
    return pfn ? pfn(force) : E_NOTIMPL;
}

typedef DWORD (WINAPI* PFN_PIXGetCaptureState)();
extern "C" DWORD WINAPI PIXGetCaptureState()
{
    static auto pfn = reinterpret_cast<PFN_PIXGetCaptureState>(RealFn("PIXGetCaptureState"));
    return pfn ? pfn() : 0;
}

typedef HRESULT (WINAPI* PFN_SetAppCompatStringPointer)(HMODULE, LPCWSTR);
extern "C" HRESULT WINAPI SetAppCompatStringPointer(HMODULE hmod, LPCWSTR name)
{
    static auto pfn = reinterpret_cast<PFN_SetAppCompatStringPointer>(RealFn("SetAppCompatStringPointer"));
    return pfn ? pfn(hmod, name) : E_NOTIMPL;
}

typedef HRESULT (WINAPI* PFN_UpdateHMDEmulationStatus)(UINT, BOOL);
extern "C" HRESULT WINAPI UpdateHMDEmulationStatus(UINT flags, BOOL enabled)
{
    static auto pfn = reinterpret_cast<PFN_UpdateHMDEmulationStatus>(RealFn("UpdateHMDEmulationStatus"));
    return pfn ? pfn(flags, enabled) : E_NOTIMPL;
}

// ---- DllMain ---------------------------------------------------------------

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInst);
        wchar_t sysDir[MAX_PATH];
        GetSystemDirectoryW(sysDir, MAX_PATH);
        wcscat_s(sysDir, MAX_PATH, L"\\dxgi.dll");
        g_hReal = LoadLibraryW(sysDir);
        WriteLog(g_hReal
            ? "[DxgiProxy] DLL_PROCESS_ATTACH: real dxgi.dll loaded OK\n"
            : "[DxgiProxy] DLL_PROCESS_ATTACH: FAILED to load real dxgi.dll\n");
        MH_Initialize();
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        MH_Uninitialize();
        if (g_hReal)
        {
            FreeLibrary(g_hReal);
            g_hReal = nullptr;
        }
    }
    return TRUE;
}
