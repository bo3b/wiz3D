/* NvDirectMode/dxgi - factory vtable hot-patch implementation (task #70)
 *
 * The hook bodies double the swap-chain desc through the d3d11 bridge,
 * unwrap the device pointer (system DXGI crashes when it walks our
 * Device11Proxy's struct internals), call the cached orig fn ptr, then
 * pass the resulting swap chain back through the d3d11 bridge so it
 * gets wrapped in a SwapChainProxy for per-eye routing.
 *
 * Recursion guard: hooks call cached orig pointers, NOT pThis->method().
 * The vtable on pThis IS the patched one — calling through it would
 * recurse into the same hook.
 */

#include "factory_vtable_hooks.h"

#pragma comment(lib, "dxguid.lib")

extern "C" void NvDM_DxgiLog(const char* fmt, ...);

namespace NvDirectMode
{

namespace
{
    // ------------------------------------------------------------------
    // IDXGIFactory / IDXGIFactory1 / IDXGIFactory2 vtable indices.
    // ------------------------------------------------------------------
    constexpr int kSlotCreateSwapChain                  = 10;
    constexpr int kSlotCreateSwapChainForHwnd           = 15;
    constexpr int kSlotCreateSwapChainForCoreWindow     = 16;
    constexpr int kSlotCreateSwapChainForComposition    = 24;

    typedef HRESULT (STDMETHODCALLTYPE *PFN_CreateSwapChain)(
        IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
    typedef HRESULT (STDMETHODCALLTYPE *PFN_CreateSwapChainForHwnd)(
        IDXGIFactory2*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**);
    typedef HRESULT (STDMETHODCALLTYPE *PFN_CreateSwapChainForCoreWindow)(
        IDXGIFactory2*, IUnknown*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*,
        IDXGIOutput*, IDXGISwapChain1**);
    typedef HRESULT (STDMETHODCALLTYPE *PFN_CreateSwapChainForComposition)(
        IDXGIFactory2*, IUnknown*, const DXGI_SWAP_CHAIN_DESC1*,
        IDXGIOutput*, IDXGISwapChain1**);

    static PFN_CreateSwapChain                  g_origCreateSwapChain                = nullptr;
    static PFN_CreateSwapChainForHwnd           g_origCreateSwapChainForHwnd         = nullptr;
    static PFN_CreateSwapChainForCoreWindow     g_origCreateSwapChainForCoreWindow   = nullptr;
    static PFN_CreateSwapChainForComposition    g_origCreateSwapChainForComposition  = nullptr;

    static bool g_patchedCreateSwapChain                = false;
    static bool g_patchedCreateSwapChainForHwnd         = false;
    static bool g_patchedCreateSwapChainForCoreWindow   = false;
    static bool g_patchedCreateSwapChainForComposition  = false;

    // ------------------------------------------------------------------
    // d3d11 cross-DLL bridge — same set of helpers DXGIFactoryProxy uses,
    // resolved once on first hook fire. We keep our own copy here rather
    // than reaching into DXGIFactoryProxy's globals so the hook bodies
    // are self-contained.
    // ------------------------------------------------------------------
    typedef void* (WINAPI *pfnNvDM_WrapAndRegisterSwapChain)(
        void* deviceOrCommandQueue, void* realSC,
        unsigned int logicalW, unsigned int logicalH);
    typedef const void* (WINAPI *pfnNvDM_DoubleSwapChainDesc)(
        const void* pDesc, unsigned int* outLogicalW, unsigned int* outLogicalH);
    typedef const void* (WINAPI *pfnNvDM_DoubleSwapChainDesc1)(
        const void* pDesc1, unsigned int* outLogicalW, unsigned int* outLogicalH);
    typedef void* (WINAPI *pfnNvDM_GetRealDevice)(void* maybeWrapped);

    static pfnNvDM_WrapAndRegisterSwapChain g_pfnWrap        = nullptr;
    static pfnNvDM_DoubleSwapChainDesc      g_pfnDoubleDesc  = nullptr;
    static pfnNvDM_DoubleSwapChainDesc1     g_pfnDoubleDesc1 = nullptr;
    static pfnNvDM_GetRealDevice            g_pfnGetReal     = nullptr;
    static volatile LONG                    g_bridgeProbed   = 0;

    void EnsureBridgeLoaded()
    {
        if (InterlockedCompareExchange(&g_bridgeProbed, 1, 0) != 0) return;
        HMODULE h = GetModuleHandleW(L"d3d11.dll");
        if (!h) return;
        g_pfnWrap        = (pfnNvDM_WrapAndRegisterSwapChain) GetProcAddress(h, "NvDM_WrapAndRegisterSwapChain");
        g_pfnDoubleDesc  = (pfnNvDM_DoubleSwapChainDesc)      GetProcAddress(h, "NvDM_DoubleSwapChainDesc");
        g_pfnDoubleDesc1 = (pfnNvDM_DoubleSwapChainDesc1)     GetProcAddress(h, "NvDM_DoubleSwapChainDesc1");
        g_pfnGetReal     = (pfnNvDM_GetRealDevice)            GetProcAddress(h, "NvDM_GetRealDevice");
        NvDM_DxgiLog("  factory_vtable_hooks bridge: d3d11=%p Wrap=%p Double=%p Double1=%p GetReal=%p\n",
                     (void*)h, (void*)g_pfnWrap, (void*)g_pfnDoubleDesc,
                     (void*)g_pfnDoubleDesc1, (void*)g_pfnGetReal);
    }

    IUnknown* UnwrapDeviceForRealCall(IUnknown* pDeviceIn, bool& outOwnsRef)
    {
        outOwnsRef = false;
        if (!g_pfnGetReal || !pDeviceIn) return pDeviceIn;
        IUnknown* real = static_cast<IUnknown*>(g_pfnGetReal(pDeviceIn));
        if (real) { outOwnsRef = true; return real; }
        return pDeviceIn;
    }

    // ------------------------------------------------------------------
    // Hooks.
    // ------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE Hook_CreateSwapChain(
        IDXGIFactory* pThis, IUnknown* pDevice,
        DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
    {
        if (!g_origCreateSwapChain) return E_FAIL;
        if (!pDesc || !ppSwapChain)
            return g_origCreateSwapChain(pThis, pDevice, pDesc, ppSwapChain);

        EnsureBridgeLoaded();

        unsigned int logicalW = pDesc->BufferDesc.Width;
        unsigned int logicalH = pDesc->BufferDesc.Height;
        DXGI_SWAP_CHAIN_DESC localDesc = *pDesc;
        const DXGI_SWAP_CHAIN_DESC* useDesc = pDesc;
        if (g_pfnDoubleDesc)
        {
            const void* doubled = g_pfnDoubleDesc(&localDesc, &logicalW, &logicalH);
            if (doubled) useDesc = static_cast<const DXGI_SWAP_CHAIN_DESC*>(doubled);
        }

        bool ownsRealRef = false;
        IUnknown* realDevice = UnwrapDeviceForRealCall(pDevice, ownsRealRef);
        HRESULT hr = g_origCreateSwapChain(pThis, realDevice,
                                            const_cast<DXGI_SWAP_CHAIN_DESC*>(useDesc),
                                            ppSwapChain);
        if (ownsRealRef && realDevice) realDevice->Release();
        NvDM_DxgiLog("  [vtable] CreateSwapChain hr=0x%08lX realSC=%p logical=%ux%u unwrapped=%d\n",
                     hr, ppSwapChain ? (void*)*ppSwapChain : nullptr, logicalW, logicalH, (int)ownsRealRef);
        if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain && g_pfnWrap)
        {
            void* wrapped = g_pfnWrap(pDevice, *ppSwapChain, logicalW, logicalH);
            if (wrapped) *ppSwapChain = static_cast<IDXGISwapChain*>(wrapped);
        }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE Hook_CreateSwapChainForHwnd(
        IDXGIFactory2* pThis, IUnknown* pDevice, HWND hWnd,
        const DXGI_SWAP_CHAIN_DESC1* pDesc,
        const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc,
        IDXGIOutput* pRestrictToOutput,
        IDXGISwapChain1** ppSwapChain)
    {
        if (!g_origCreateSwapChainForHwnd) return E_FAIL;
        if (!pDesc || !ppSwapChain)
            return g_origCreateSwapChainForHwnd(pThis, pDevice, hWnd, pDesc,
                                                 pFullscreenDesc, pRestrictToOutput, ppSwapChain);

        EnsureBridgeLoaded();

        unsigned int logicalW = pDesc->Width;
        unsigned int logicalH = pDesc->Height;
        DXGI_SWAP_CHAIN_DESC1 localDesc = *pDesc;
        const DXGI_SWAP_CHAIN_DESC1* useDesc = pDesc;
        if (g_pfnDoubleDesc1)
        {
            const void* doubled = g_pfnDoubleDesc1(&localDesc, &logicalW, &logicalH);
            if (doubled) useDesc = static_cast<const DXGI_SWAP_CHAIN_DESC1*>(doubled);
        }

        bool ownsRealRef = false;
        IUnknown* realDevice = UnwrapDeviceForRealCall(pDevice, ownsRealRef);
        HRESULT hr = g_origCreateSwapChainForHwnd(pThis, realDevice, hWnd,
                                                   useDesc, pFullscreenDesc,
                                                   pRestrictToOutput, ppSwapChain);
        if (ownsRealRef && realDevice) realDevice->Release();
        NvDM_DxgiLog("  [vtable] CreateSwapChainForHwnd hr=0x%08lX realSC=%p logical=%ux%u unwrapped=%d\n",
                     hr, ppSwapChain ? (void*)*ppSwapChain : nullptr, logicalW, logicalH, (int)ownsRealRef);
        if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain && g_pfnWrap)
        {
            void* wrapped = g_pfnWrap(pDevice, *ppSwapChain, logicalW, logicalH);
            if (wrapped) *ppSwapChain = static_cast<IDXGISwapChain1*>(wrapped);
        }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE Hook_CreateSwapChainForCoreWindow(
        IDXGIFactory2* pThis, IUnknown* pDevice, IUnknown* pWindow,
        const DXGI_SWAP_CHAIN_DESC1* pDesc,
        IDXGIOutput* pRestrictToOutput,
        IDXGISwapChain1** ppSwapChain)
    {
        if (!g_origCreateSwapChainForCoreWindow) return E_FAIL;
        if (!pDesc || !ppSwapChain)
            return g_origCreateSwapChainForCoreWindow(pThis, pDevice, pWindow, pDesc,
                                                       pRestrictToOutput, ppSwapChain);

        EnsureBridgeLoaded();

        unsigned int logicalW = pDesc->Width;
        unsigned int logicalH = pDesc->Height;
        DXGI_SWAP_CHAIN_DESC1 localDesc = *pDesc;
        const DXGI_SWAP_CHAIN_DESC1* useDesc = pDesc;
        if (g_pfnDoubleDesc1)
        {
            const void* doubled = g_pfnDoubleDesc1(&localDesc, &logicalW, &logicalH);
            if (doubled) useDesc = static_cast<const DXGI_SWAP_CHAIN_DESC1*>(doubled);
        }

        bool ownsRealRef = false;
        IUnknown* realDevice = UnwrapDeviceForRealCall(pDevice, ownsRealRef);
        HRESULT hr = g_origCreateSwapChainForCoreWindow(pThis, realDevice, pWindow,
                                                         useDesc, pRestrictToOutput, ppSwapChain);
        if (ownsRealRef && realDevice) realDevice->Release();
        if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain && g_pfnWrap)
        {
            void* wrapped = g_pfnWrap(pDevice, *ppSwapChain, logicalW, logicalH);
            if (wrapped) *ppSwapChain = static_cast<IDXGISwapChain1*>(wrapped);
        }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE Hook_CreateSwapChainForComposition(
        IDXGIFactory2* pThis, IUnknown* pDevice,
        const DXGI_SWAP_CHAIN_DESC1* pDesc,
        IDXGIOutput* pRestrictToOutput,
        IDXGISwapChain1** ppSwapChain)
    {
        if (!g_origCreateSwapChainForComposition) return E_FAIL;
        if (!pDesc || !ppSwapChain)
            return g_origCreateSwapChainForComposition(pThis, pDevice, pDesc,
                                                        pRestrictToOutput, ppSwapChain);

        EnsureBridgeLoaded();

        unsigned int logicalW = pDesc->Width;
        unsigned int logicalH = pDesc->Height;
        DXGI_SWAP_CHAIN_DESC1 localDesc = *pDesc;
        const DXGI_SWAP_CHAIN_DESC1* useDesc = pDesc;
        if (g_pfnDoubleDesc1)
        {
            const void* doubled = g_pfnDoubleDesc1(&localDesc, &logicalW, &logicalH);
            if (doubled) useDesc = static_cast<const DXGI_SWAP_CHAIN_DESC1*>(doubled);
        }

        bool ownsRealRef = false;
        IUnknown* realDevice = UnwrapDeviceForRealCall(pDevice, ownsRealRef);
        HRESULT hr = g_origCreateSwapChainForComposition(pThis, realDevice,
                                                          useDesc, pRestrictToOutput, ppSwapChain);
        if (ownsRealRef && realDevice) realDevice->Release();
        if (SUCCEEDED(hr) && ppSwapChain && *ppSwapChain && g_pfnWrap)
        {
            void* wrapped = g_pfnWrap(pDevice, *ppSwapChain, logicalW, logicalH);
            if (wrapped) *ppSwapChain = static_cast<IDXGISwapChain1*>(wrapped);
        }
        return hr;
    }

    bool PatchSlot(void** vtable, int slot, void* hook, void** outOriginal, const char* tag)
    {
        DWORD oldProtect = 0;
        if (!VirtualProtect(&vtable[slot], sizeof(void*), PAGE_READWRITE, &oldProtect))
        {
            NvDM_DxgiLog("  FactoryVtablePatch[%s, slot=%d]: VirtualProtect RW failed err=%lu\n",
                         tag, slot, GetLastError());
            return false;
        }
        *outOriginal = vtable[slot];
        vtable[slot] = hook;
        DWORD restored = 0;
        VirtualProtect(&vtable[slot], sizeof(void*), oldProtect, &restored);
        NvDM_DxgiLog("  FactoryVtablePatch[%s] slot=%d: orig=%p -> hook=%p\n",
                     tag, slot, *outOriginal, hook);
        return true;
    }
} // anonymous namespace

bool InstallFactoryVtablePatch(IDXGIFactory* r0, IDXGIFactory2* r2)
{
    if (!r0) return false;

    // IDXGIFactory base slots — present on every factory the runtime can
    // hand out. Vtable is shared across instances; patching once covers
    // any future IDXGIFactory* the game gets (including via GetParent).
    void** vt0 = *reinterpret_cast<void***>(r0);
    if (!g_patchedCreateSwapChain)
    {
        if (PatchSlot(vt0, kSlotCreateSwapChain,
                      reinterpret_cast<void*>(&Hook_CreateSwapChain),
                      reinterpret_cast<void**>(&g_origCreateSwapChain),
                      "CreateSwapChain"))
            g_patchedCreateSwapChain = true;
    }

    // IDXGIFactory2 slots — only patched if r2 is available. The vtable
    // for the IDXGIFactory2 face is the SAME memory as IDXGIFactory0 when
    // single inheritance + extension applies (which it does for DXGI). So
    // the slots beyond IDXGIFactory1 land at the same vtable indices on
    // r0's vtable. Belt-and-braces: fetch r2's vtable explicitly anyway
    // in case some host implementation differs.
    if (r2)
    {
        void** vt2 = *reinterpret_cast<void***>(r2);
        if (!g_patchedCreateSwapChainForHwnd)
        {
            if (PatchSlot(vt2, kSlotCreateSwapChainForHwnd,
                          reinterpret_cast<void*>(&Hook_CreateSwapChainForHwnd),
                          reinterpret_cast<void**>(&g_origCreateSwapChainForHwnd),
                          "CreateSwapChainForHwnd"))
                g_patchedCreateSwapChainForHwnd = true;
        }
        if (!g_patchedCreateSwapChainForCoreWindow)
        {
            if (PatchSlot(vt2, kSlotCreateSwapChainForCoreWindow,
                          reinterpret_cast<void*>(&Hook_CreateSwapChainForCoreWindow),
                          reinterpret_cast<void**>(&g_origCreateSwapChainForCoreWindow),
                          "CreateSwapChainForCoreWindow"))
                g_patchedCreateSwapChainForCoreWindow = true;
        }
        if (!g_patchedCreateSwapChainForComposition)
        {
            if (PatchSlot(vt2, kSlotCreateSwapChainForComposition,
                          reinterpret_cast<void*>(&Hook_CreateSwapChainForComposition),
                          reinterpret_cast<void**>(&g_origCreateSwapChainForComposition),
                          "CreateSwapChainForComposition"))
                g_patchedCreateSwapChainForComposition = true;
        }
    }
    return true;
}

} // namespace NvDirectMode
