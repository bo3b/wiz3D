/* NvDirectMode/d3d9 - layout-stable IDirect3D9 vtable hot-patch
 *
 * Task #61 — bypass GTA IV / Hard Reset's anti-tamper sniff that crashes
 * the game immediately after Direct3DCreate9 returns when our wrapped
 * D3D9Proxy is in place. The sniff reads bytes out of the IDirect3D9
 * implementation memory (vtable pointer, vtable contents, possibly
 * private fields) looking for NVIDIA-d3d9 signatures; our proxy class
 * has a different vtable address and different function pointers, so it
 * fails.
 *
 * Approach: instead of returning our D3D9Proxy from Direct3DCreate9,
 * return the *real* IDirect3D9 with its CreateDevice (slot 16) and
 * CreateDeviceEx (slot 20, when Ex) hot-patched to our hooks. The sniff
 * sees the real layout — passes. When the game later calls
 * iD3D9->CreateDevice via the patched vtable, our hook fires and we
 * wrap the returned device with the existing Device9Proxy.
 *
 * Limitations:
 *   - Vtable patches are *global* to the process. All IDirect3D9
 *     instances in this process route through our hook. Single-game
 *     processes are the common case (TR / GTA IV / Hard Reset all create
 *     one).
 *   - If the game also sniffs IDirect3DDevice9 after CreateDevice
 *     returns, we'd need to extend the patch to the device's vtable
 *     (~50 method slots). Punted until we see a game that needs it.
 *   - Anti-tamper that compares vtable function addresses against
 *     d3d9.dll's address range will still flag our hooks (which live
 *     inside our proxy DLL). Real fix would need Detours-style inline
 *     patching with a trampoline inside d3d9.dll's code section.
 *
 * Opt-in via 3DVision_Config.xml's UseLayoutStableProxy=1 — default OFF
 * so games that already work with the existing D3D9Proxy approach
 * (Tutorial07 etc.) aren't affected.
 */

#include "proxy_factory.h"
#include "Device9Proxy.h"
#include "device_vtable_hooks.h"
#include "swapchain_helpers.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

extern "C" void NvDM_Log(const char* fmt, ...);
extern "C" int  NvDM_VerboseEnabled();
extern "C" int  NvDM_UseLayoutStableLevel();   // 0 / 1 / 2

namespace NvDirectMode
{

namespace
{
    // IDirect3D9 / IDirect3D9Ex vtable indices (per d3d9.h IDL — stable
    // across Windows versions and SDK revisions).
    //
    // IDirect3D9 ends at slot 16 (CreateDevice). IDirect3D9Ex extends:
    //   17: GetAdapterModeCountEx
    //   18: EnumAdapterModesEx
    //   19: GetAdapterDisplayModeEx
    //   20: CreateDeviceEx          <-- slot we want
    //   21: GetAdapterLUID
    //
    // Earlier versions of this file had kSlotCreateDeviceEx = 19 (off by
    // one — that's GetAdapterDisplayModeEx). Hard Reset / GTA IV / AC
    // Revelations didn't hit this because they all use the non-Ex
    // Direct3DCreate9. Bo3b's exp_dx9_dm Tutorial07 uses Direct3DCreate9Ex
    // and crashed inside system d3d9.dll the first time we ran it because
    // the game's GetAdapterDisplayModeEx call hit our hook expecting
    // CreateDeviceEx's signature.
    constexpr int kSlotCreateDevice    = 16;   // IDirect3D9::CreateDevice
    constexpr int kSlotCreateDeviceEx  = 20;   // IDirect3D9Ex::CreateDeviceEx

    typedef HRESULT (STDMETHODCALLTYPE *PFN_CreateDevice)(
        IDirect3D9*, UINT, D3DDEVTYPE, HWND, DWORD,
        D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
    typedef HRESULT (STDMETHODCALLTYPE *PFN_CreateDeviceEx)(
        IDirect3D9Ex*, UINT, D3DDEVTYPE, HWND, DWORD,
        D3DPRESENT_PARAMETERS*, D3DDISPLAYMODEEX*, IDirect3DDevice9Ex**);

    static PFN_CreateDevice   g_origCreateDevice   = nullptr;
    static PFN_CreateDeviceEx g_origCreateDeviceEx = nullptr;
    static bool g_patchedCreateDevice   = false;
    static bool g_patchedCreateDeviceEx = false;

    HRESULT STDMETHODCALLTYPE Hook_CreateDevice(
        IDirect3D9* pThis,
        UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
        DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPP,
        IDirect3DDevice9** ppReturnedDeviceInterface)
    {
        if (!ppReturnedDeviceInterface || !g_origCreateDevice)
            return D3DERR_INVALIDCALL;

        D3DPRESENT_PARAMETERS modified;
        D3DPRESENT_PARAMETERS* pParams = pPP;
        UINT logicalW = 0, logicalH = 0;
        if (pParams)
        {
            modified = *pParams;
            ResolveAndDoubleSwapchainParams(&modified, hFocusWindow, &logicalW, &logicalH);
            pParams = &modified;
        }

        IDirect3DDevice9* realDevice = nullptr;
        HRESULT hr = g_origCreateDevice(pThis, Adapter, DeviceType, hFocusWindow,
                                         BehaviorFlags, pParams, &realDevice);
        if (FAILED(hr) || !realDevice)
        {
            NvDM_Log("  Hook_CreateDevice: real CreateDevice failed hr=0x%08lX\n", hr);
            *ppReturnedDeviceInterface = nullptr;
            return hr;
        }
        // Task #68: when UseLayoutStableProxy=2, hot-patch the real
        // device's vtable too and hand the game the real pointer. Skips
        // Device9Proxy entirely so the game's anti-tamper sniff of the
        // device internals (GTA IV's Reset() validator) sees real layout.
        if (NvDM_UseLayoutStableLevel() >= 2 &&
            InstallDeviceVtablePatch(realDevice, /*isEx=*/false, logicalW, logicalH))
        {
            NvDM_Log("  Hook_CreateDevice: device vtable patched, returning real %p (logical=%ux%u)\n",
                     realDevice, logicalW, logicalH);
            *ppReturnedDeviceInterface = realDevice;
            return hr;
        }
        auto* proxy = new Device9Proxy(realDevice, /*isEx=*/false);
        if (logicalW > 0) proxy->SetLogicalBackBufferSize(logicalW, logicalH);
        proxy->StashBackBufferReference();
        *ppReturnedDeviceInterface = proxy;
        NvDM_Log("  Hook_CreateDevice: wrapped real=%p -> Device9Proxy=%p (logical=%ux%u)\n",
                 realDevice, (void*)proxy, logicalW, logicalH);
        return hr;
    }

    HRESULT STDMETHODCALLTYPE Hook_CreateDeviceEx(
        IDirect3D9Ex* pThis,
        UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
        DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPP,
        D3DDISPLAYMODEEX* pFullscreenDisplayMode,
        IDirect3DDevice9Ex** ppReturnedDeviceInterface)
    {
        if (!ppReturnedDeviceInterface || !g_origCreateDeviceEx)
            return D3DERR_INVALIDCALL;

        D3DPRESENT_PARAMETERS modified;
        D3DPRESENT_PARAMETERS* pParams = pPP;
        UINT logicalW = 0, logicalH = 0;
        if (pParams)
        {
            modified = *pParams;
            ResolveAndDoubleSwapchainParams(&modified, hFocusWindow, &logicalW, &logicalH);
            pParams = &modified;
        }

        IDirect3DDevice9Ex* realDevice = nullptr;
        HRESULT hr = g_origCreateDeviceEx(pThis, Adapter, DeviceType, hFocusWindow,
                                           BehaviorFlags, pParams,
                                           pFullscreenDisplayMode, &realDevice);
        if (FAILED(hr) || !realDevice)
        {
            NvDM_Log("  Hook_CreateDeviceEx: real CreateDeviceEx failed hr=0x%08lX\n", hr);
            *ppReturnedDeviceInterface = nullptr;
            return hr;
        }
        if (NvDM_UseLayoutStableLevel() >= 2 &&
            InstallDeviceVtablePatch(realDevice, /*isEx=*/true, logicalW, logicalH))
        {
            NvDM_Log("  Hook_CreateDeviceEx: device vtable patched, returning real %p (logical=%ux%u)\n",
                     realDevice, logicalW, logicalH);
            *ppReturnedDeviceInterface = realDevice;
            return hr;
        }
        auto* proxy = new Device9Proxy(realDevice, /*isEx=*/true);
        if (logicalW > 0) proxy->SetLogicalBackBufferSize(logicalW, logicalH);
        proxy->StashBackBufferReference();
        *ppReturnedDeviceInterface = static_cast<IDirect3DDevice9Ex*>(proxy);
        NvDM_Log("  Hook_CreateDeviceEx: wrapped real=%p -> Device9Proxy=%p (logical=%ux%u)\n",
                 realDevice, (void*)proxy, logicalW, logicalH);
        return hr;
    }

    bool PatchVtableSlot(void** vtable, int slot, void* hook, void** outOriginal)
    {
        DWORD oldProtect = 0;
        if (!VirtualProtect(&vtable[slot], sizeof(void*), PAGE_READWRITE, &oldProtect))
        {
            NvDM_Log("  PatchVtableSlot(slot=%d): VirtualProtect RW failed err=%lu\n",
                     slot, GetLastError());
            return false;
        }
        *outOriginal = vtable[slot];
        vtable[slot] = hook;
        DWORD restored = 0;
        VirtualProtect(&vtable[slot], sizeof(void*), oldProtect, &restored);
        NvDM_Log("  PatchVtableSlot(slot=%d): orig=%p -> hook=%p\n",
                 slot, *outOriginal, hook);
        return true;
    }
} // anonymous namespace

bool InstallVtablePatchForD3D9(void* realD3D9, bool isEx)
{
    if (!realD3D9) return false;

    // Vtable pointer is the first member of any COM object — single-
    // inheritance COM means *(void***) hits the vtable immediately.
    void** vtable = *reinterpret_cast<void***>(realD3D9);

    if (!g_patchedCreateDevice)
    {
        if (!PatchVtableSlot(vtable, kSlotCreateDevice,
                             reinterpret_cast<void*>(&Hook_CreateDevice),
                             reinterpret_cast<void**>(&g_origCreateDevice)))
            return false;
        g_patchedCreateDevice = true;
    }

    if (isEx && !g_patchedCreateDeviceEx)
    {
        if (!PatchVtableSlot(vtable, kSlotCreateDeviceEx,
                             reinterpret_cast<void*>(&Hook_CreateDeviceEx),
                             reinterpret_cast<void**>(&g_origCreateDeviceEx)))
            return false;
        g_patchedCreateDeviceEx = true;
    }

    return true;
}

} // namespace NvDirectMode
