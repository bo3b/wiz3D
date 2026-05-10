/* NvDirectMode/d3d9 - IDirect3DDevice9 vtable hot-patch (task #68)
 *
 * See device_vtable_hooks.h for high-level design.
 *
 * Recursion guard: when a hook calls back into the real device (e.g. our
 * Hook_Present needs to forward to the original Present), we MUST NOT go
 * via the device's vtable — the vtable is patched, so we'd recurse into
 * ourselves. We use cached PFN_* pointers captured at install time and
 * call those directly with the real device as 'this'. Same model as
 * dxgi/adapter_vtable_hooks.cpp.
 */

#include "device_vtable_hooks.h"
#include "swapchain_helpers.h"
#include "eye_state.h"
#include "log.h"

extern "C" void NvDM_Log(const char* fmt, ...);
extern "C" int  NvDM_VerboseEnabled();
extern "C" int  NvDM_OutputIsTopBottom();
extern "C" int  NvDM_SwapEyes();

namespace NvDirectMode
{

namespace
{
    // ------------------------------------------------------------------
    // IDirect3DDevice9 vtable indices (stable across Windows versions
    // since the IDL never changes for shipped d3d9.h interfaces).
    // ------------------------------------------------------------------
    constexpr int kSlotReset         = 16;
    constexpr int kSlotPresent       = 17;
    constexpr int kSlotGetBackBuffer = 18;
    // IDirect3DDevice9Ex extends IDirect3DDevice9 (119 slots: 0-118),
    // then appends its own. PresentEx = 121, ResetEx = 132.
    constexpr int kSlotPresentEx     = 121;
    constexpr int kSlotResetEx       = 132;

    typedef HRESULT (STDMETHODCALLTYPE *PFN_Reset)        (IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
    typedef HRESULT (STDMETHODCALLTYPE *PFN_Present)      (IDirect3DDevice9*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*);
    typedef HRESULT (STDMETHODCALLTYPE *PFN_GetBackBuffer)(IDirect3DDevice9*, UINT, UINT, D3DBACKBUFFER_TYPE, IDirect3DSurface9**);
    typedef HRESULT (STDMETHODCALLTYPE *PFN_PresentEx)    (IDirect3DDevice9Ex*, CONST RECT*, CONST RECT*, HWND, CONST RGNDATA*, DWORD);
    typedef HRESULT (STDMETHODCALLTYPE *PFN_ResetEx)      (IDirect3DDevice9Ex*, D3DPRESENT_PARAMETERS*, D3DDISPLAYMODEEX*);

    static PFN_Reset         g_origReset         = nullptr;
    static PFN_Present       g_origPresent       = nullptr;
    static PFN_GetBackBuffer g_origGetBackBuffer = nullptr;
    static PFN_PresentEx     g_origPresentEx     = nullptr;
    static PFN_ResetEx       g_origResetEx       = nullptr;

    static bool g_patchedReset         = false;
    static bool g_patchedPresent       = false;
    static bool g_patchedGetBackBuffer = false;
    static bool g_patchedPresentEx     = false;
    static bool g_patchedResetEx       = false;

    // ------------------------------------------------------------------
    // Primary device shadow state. One game = one device, same as the
    // Device9Proxy::g_primaryDevice slot — keeps the mapping trivial
    // (no map lookup per Present) and matches what the eye-change
    // callback dispatcher already assumes.
    // ------------------------------------------------------------------
    struct ShadowState
    {
        IDirect3DDevice9*    realDevice    = nullptr;     // not refed (game owns it)
        IDirect3DDevice9Ex*  realDeviceEx  = nullptr;     // == realDevice when isEx
        bool                 isEx          = false;
        UINT                 logicalW      = 0;
        UINT                 logicalH      = 0;
        IDirect3DSurface9*   trackedBB     = nullptr;     // refed
        IDirect3DSurface9*   shadowBB      = nullptr;     // refed
        IDirect3DSurface9*   leftEye       = nullptr;     // refed
        IDirect3DSurface9*   rightEye      = nullptr;     // refed
    };

    static ShadowState      g_state;
    static CRITICAL_SECTION g_stateLock;
    static volatile LONG    g_stateLockInit = 0;

    void EnsureStateLock()
    {
        if (InterlockedCompareExchange(&g_stateLockInit, 1, 0) == 0)
            InitializeCriticalSection(&g_stateLock);
    }

    void ReleaseShadowSurfaces(ShadowState& s)
    {
        if (s.shadowBB) { s.shadowBB->Release(); s.shadowBB = nullptr; }
        if (s.leftEye)  { s.leftEye->Release();  s.leftEye  = nullptr; }
        if (s.rightEye) { s.rightEye->Release(); s.rightEye = nullptr; }
    }

    void ReleaseTrackedBB(ShadowState& s)
    {
        if (s.trackedBB) { s.trackedBB->Release(); s.trackedBB = nullptr; }
    }

    void EnsureShadow(ShadowState& s)
    {
        if (s.shadowBB || !s.realDevice || !s.trackedBB) return;
        D3DSURFACE_DESC desc = {};
        if (FAILED(s.trackedBB->GetDesc(&desc))) return;
        s.logicalW = desc.Width;
        s.logicalH = desc.Height;
        // Game-side render target at logical size, same format/MS so
        // StretchRect can copy without conversion.
        HRESULT hr = s.realDevice->CreateRenderTarget(desc.Width, desc.Height, desc.Format,
                                                       desc.MultiSampleType, desc.MultiSampleQuality,
                                                       FALSE, &s.shadowBB, NULL);
        if (FAILED(hr) || !s.shadowBB)
        {
            NvDM_Log("  d3d9 [stable-dev] EnsureShadow: CreateRenderTarget(%ux%u) FAILED hr=0x%08lX\n",
                     desc.Width, desc.Height, hr);
            s.shadowBB = nullptr;
            return;
        }
        NvDM_Log("  d3d9 [stable-dev] EnsureShadow: shadow=%p (%ux%u, fmt=%d) for realBB=%p\n",
                 s.shadowBB, desc.Width, desc.Height, (int)desc.Format, (void*)s.trackedBB);
    }

    void StashTrackedBB(ShadowState& s)
    {
        ReleaseTrackedBB(s);
        ReleaseShadowSurfaces(s);
        if (!s.realDevice) return;
        // GetBackBuffer(0,0,MONO) — go via cached orig to avoid recursion
        // into our own Hook_GetBackBuffer (which would hand back the
        // shadow we haven't created yet).
        if (g_origGetBackBuffer)
            g_origGetBackBuffer(s.realDevice, 0, 0, D3DBACKBUFFER_TYPE_MONO, &s.trackedBB);
        EnsureShadow(s);
    }

    void CaptureEye(ShadowState& s, int eyeBeingLeft)
    {
        if (!s.shadowBB || !s.realDevice) return;
        IDirect3DSurface9** slot = nullptr;
        if      (eyeBeingLeft == NvDirectMode::kEyeLeft)  slot = &s.leftEye;
        else if (eyeBeingLeft == NvDirectMode::kEyeRight) slot = &s.rightEye;
        else return;

        if (!*slot)
        {
            D3DSURFACE_DESC desc = {};
            s.shadowBB->GetDesc(&desc);
            HRESULT hr = s.realDevice->CreateRenderTarget(desc.Width, desc.Height, desc.Format,
                                                           desc.MultiSampleType, desc.MultiSampleQuality,
                                                           FALSE, slot, NULL);
            if (FAILED(hr) || !*slot) return;
            NvDM_Log("  d3d9 [stable-dev] CaptureEye(%d): allocated eye surface=%p\n",
                     eyeBeingLeft, *slot);
        }
        s.realDevice->StretchRect(s.shadowBB, NULL, *slot, NULL, D3DTEXF_NONE);
    }

    void CompositeIntoRealBB(ShadowState& s)
    {
        if (!s.realDevice || !s.trackedBB) return;
        int currentEye = NvDirectMode::GetActiveEye();
        if (currentEye == NvDirectMode::kEyeLeft || currentEye == NvDirectMode::kEyeRight)
            CaptureEye(s, currentEye);

        bool topBottom = NvDM_OutputIsTopBottom() != 0;
        bool swap      = NvDM_SwapEyes() != 0;
        IDirect3DSurface9* leftSrc  = swap ? s.rightEye : s.leftEye;
        IDirect3DSurface9* rightSrc = swap ? s.leftEye  : s.rightEye;

        D3DSURFACE_DESC bbDesc = {};
        s.trackedBB->GetDesc(&bbDesc);

        if (leftSrc && rightSrc)
        {
            RECT leftRect, rightRect;
            if (topBottom)
            {
                leftRect  = { 0, 0, (LONG)bbDesc.Width, (LONG)(bbDesc.Height / 2) };
                rightRect = { 0, (LONG)(bbDesc.Height / 2), (LONG)bbDesc.Width, (LONG)bbDesc.Height };
            }
            else
            {
                leftRect  = { 0, 0, (LONG)(bbDesc.Width / 2), (LONG)bbDesc.Height };
                rightRect = { (LONG)(bbDesc.Width / 2), 0, (LONG)bbDesc.Width, (LONG)bbDesc.Height };
            }
            s.realDevice->StretchRect(leftSrc,  NULL, s.trackedBB, &leftRect,  D3DTEXF_LINEAR);
            s.realDevice->StretchRect(rightSrc, NULL, s.trackedBB, &rightRect, D3DTEXF_LINEAR);
        }
        else
        {
            // Single eye / mono fallback.
            IDirect3DSurface9* src = leftSrc ? leftSrc : (rightSrc ? rightSrc : s.shadowBB);
            if (src)
                s.realDevice->StretchRect(src, NULL, s.trackedBB, NULL, D3DTEXF_NONE);
        }
    }

    // ------------------------------------------------------------------
    // Eye-change callback. Mirrors Device9Proxy::OnEyeChange — captures
    // the OLD eye's frame from the shadow into a per-eye surface.
    // ------------------------------------------------------------------
    void OnEyeChange(int oldEye, int /*newEye*/)
    {
        EnsureStateLock();
        EnterCriticalSection(&g_stateLock);
        if (g_state.realDevice) CaptureEye(g_state, oldEye);
        LeaveCriticalSection(&g_stateLock);
    }

    // ------------------------------------------------------------------
    // Hooks. pThis is the real device (game's pointer). Look up shadow
    // state, do our work, forward via cached orig fn ptr.
    // ------------------------------------------------------------------
    HRESULT STDMETHODCALLTYPE Hook_Reset(IDirect3DDevice9* pThis, D3DPRESENT_PARAMETERS* pPP)
    {
        if (!g_origReset) return D3DERR_INVALIDCALL;

        EnsureStateLock();
        EnterCriticalSection(&g_stateLock);

        // Tracked BB & shadow are invalidated by Reset — release before
        // forwarding so refs don't outlive the swapchain rebuild.
        if (pThis == g_state.realDevice)
        {
            ReleaseTrackedBB(g_state);
            ReleaseShadowSurfaces(g_state);
        }

        D3DPRESENT_PARAMETERS modified;
        UINT logicalW = 0, logicalH = 0;
        if (pPP)
        {
            modified = *pPP;
            ResolveAndDoubleSwapchainParams(&modified, modified.hDeviceWindow, &logicalW, &logicalH);
        }

        LeaveCriticalSection(&g_stateLock);
        HRESULT hr = g_origReset(pThis, pPP ? &modified : nullptr);
        EnterCriticalSection(&g_stateLock);

        if (SUCCEEDED(hr) && pThis == g_state.realDevice)
        {
            if (logicalW > 0) { g_state.logicalW = logicalW; g_state.logicalH = logicalH; }
            StashTrackedBB(g_state);
        }
        else if (FAILED(hr))
        {
            NvDM_Log("  d3d9 [stable-dev] Hook_Reset: orig Reset failed hr=0x%08lX\n", hr);
        }
        LeaveCriticalSection(&g_stateLock);
        return hr;
    }

    HRESULT STDMETHODCALLTYPE Hook_Present(IDirect3DDevice9* pThis,
                                            CONST RECT* sr, CONST RECT* dr,
                                            HWND h, CONST RGNDATA* d)
    {
        if (!g_origPresent) return D3DERR_INVALIDCALL;
        EnsureStateLock();
        EnterCriticalSection(&g_stateLock);
        if (pThis == g_state.realDevice) CompositeIntoRealBB(g_state);
        LeaveCriticalSection(&g_stateLock);
        return g_origPresent(pThis, sr, dr, h, d);
    }

    HRESULT STDMETHODCALLTYPE Hook_GetBackBuffer(IDirect3DDevice9* pThis,
                                                  UINT iSC, UINT iBB,
                                                  D3DBACKBUFFER_TYPE T,
                                                  IDirect3DSurface9** ppBB)
    {
        if (!g_origGetBackBuffer) return D3DERR_INVALIDCALL;
        // Hand the shadow surface for the primary BB; everything else
        // forwards untouched.
        if (iSC == 0 && iBB == 0 && T == D3DBACKBUFFER_TYPE_MONO && ppBB)
        {
            EnsureStateLock();
            EnterCriticalSection(&g_stateLock);
            if (pThis == g_state.realDevice)
            {
                EnsureShadow(g_state);
                if (g_state.shadowBB)
                {
                    g_state.shadowBB->AddRef();
                    *ppBB = g_state.shadowBB;
                    LeaveCriticalSection(&g_stateLock);
                    return S_OK;
                }
            }
            LeaveCriticalSection(&g_stateLock);
        }
        return g_origGetBackBuffer(pThis, iSC, iBB, T, ppBB);
    }

    HRESULT STDMETHODCALLTYPE Hook_PresentEx(IDirect3DDevice9Ex* pThis,
                                              CONST RECT* sr, CONST RECT* dr,
                                              HWND h, CONST RGNDATA* d, DWORD F)
    {
        if (!g_origPresentEx) return D3DERR_INVALIDCALL;
        EnsureStateLock();
        EnterCriticalSection(&g_stateLock);
        if (static_cast<IDirect3DDevice9*>(pThis) == g_state.realDevice)
            CompositeIntoRealBB(g_state);
        LeaveCriticalSection(&g_stateLock);
        return g_origPresentEx(pThis, sr, dr, h, d, F);
    }

    HRESULT STDMETHODCALLTYPE Hook_ResetEx(IDirect3DDevice9Ex* pThis,
                                            D3DPRESENT_PARAMETERS* pPP,
                                            D3DDISPLAYMODEEX* pFM)
    {
        if (!g_origResetEx) return D3DERR_INVALIDCALL;

        EnsureStateLock();
        EnterCriticalSection(&g_stateLock);
        if (static_cast<IDirect3DDevice9*>(pThis) == g_state.realDevice)
        {
            ReleaseTrackedBB(g_state);
            ReleaseShadowSurfaces(g_state);
        }

        D3DPRESENT_PARAMETERS modified;
        UINT logicalW = 0, logicalH = 0;
        if (pPP)
        {
            modified = *pPP;
            ResolveAndDoubleSwapchainParams(&modified, modified.hDeviceWindow, &logicalW, &logicalH);
        }

        LeaveCriticalSection(&g_stateLock);
        HRESULT hr = g_origResetEx(pThis, pPP ? &modified : nullptr, pFM);
        EnterCriticalSection(&g_stateLock);

        if (SUCCEEDED(hr) && static_cast<IDirect3DDevice9*>(pThis) == g_state.realDevice)
        {
            if (logicalW > 0) { g_state.logicalW = logicalW; g_state.logicalH = logicalH; }
            StashTrackedBB(g_state);
        }
        else if (FAILED(hr))
        {
            NvDM_Log("  d3d9 [stable-dev] Hook_ResetEx: orig ResetEx failed hr=0x%08lX\n", hr);
        }
        LeaveCriticalSection(&g_stateLock);
        return hr;
    }

    bool PatchSlot(void** vtable, int slot, void* hook, void** outOriginal, const char* tag)
    {
        DWORD oldProtect = 0;
        if (!VirtualProtect(&vtable[slot], sizeof(void*), PAGE_READWRITE, &oldProtect))
        {
            NvDM_Log("  d3d9 [stable-dev] PatchSlot(%s, slot=%d): VirtualProtect RW failed err=%lu\n",
                     tag, slot, GetLastError());
            return false;
        }
        *outOriginal = vtable[slot];
        vtable[slot] = hook;
        DWORD restored = 0;
        VirtualProtect(&vtable[slot], sizeof(void*), oldProtect, &restored);
        NvDM_Log("  d3d9 [stable-dev] PatchSlot(%s, slot=%d): orig=%p -> hook=%p\n",
                 tag, slot, *outOriginal, hook);
        return true;
    }
} // anonymous namespace

bool InstallDeviceVtablePatch(IDirect3DDevice9* realDevice, bool isEx,
                                UINT logicalW, UINT logicalH)
{
    if (!realDevice) return false;

    EnsureStateLock();

    // Patch vtable slots once per process. The vtable is shared across
    // every IDirect3DDevice9 in this process (system-runtime singleton),
    // so a single patch covers any future devices the game creates.
    void** vtable = *reinterpret_cast<void***>(realDevice);

    if (!g_patchedReset)
    {
        if (!PatchSlot(vtable, kSlotReset,
                       reinterpret_cast<void*>(&Hook_Reset),
                       reinterpret_cast<void**>(&g_origReset),
                       "Reset"))
            return false;
        g_patchedReset = true;
    }
    if (!g_patchedPresent)
    {
        if (!PatchSlot(vtable, kSlotPresent,
                       reinterpret_cast<void*>(&Hook_Present),
                       reinterpret_cast<void**>(&g_origPresent),
                       "Present"))
            return false;
        g_patchedPresent = true;
    }
    if (!g_patchedGetBackBuffer)
    {
        if (!PatchSlot(vtable, kSlotGetBackBuffer,
                       reinterpret_cast<void*>(&Hook_GetBackBuffer),
                       reinterpret_cast<void**>(&g_origGetBackBuffer),
                       "GetBackBuffer"))
            return false;
        g_patchedGetBackBuffer = true;
    }

    if (isEx)
    {
        // The Ex slots live on the same vtable when the device's true type
        // is IDirect3DDevice9Ex (single inheritance + extension).
        if (!g_patchedPresentEx)
        {
            if (PatchSlot(vtable, kSlotPresentEx,
                          reinterpret_cast<void*>(&Hook_PresentEx),
                          reinterpret_cast<void**>(&g_origPresentEx),
                          "PresentEx"))
                g_patchedPresentEx = true;
        }
        if (!g_patchedResetEx)
        {
            if (PatchSlot(vtable, kSlotResetEx,
                          reinterpret_cast<void*>(&Hook_ResetEx),
                          reinterpret_cast<void**>(&g_origResetEx),
                          "ResetEx"))
                g_patchedResetEx = true;
        }
    }

    // Register state for this device. Eye-change callback is registered
    // once; the dispatcher only fires for our primary device.
    EnterCriticalSection(&g_stateLock);
    if (g_state.realDevice && g_state.realDevice != realDevice)
    {
        // Replacing primary — release old refs (game presumably destroyed
        // the old device and is making a new one).
        ReleaseTrackedBB(g_state);
        ReleaseShadowSurfaces(g_state);
    }
    g_state.realDevice   = realDevice;
    g_state.realDeviceEx = isEx ? static_cast<IDirect3DDevice9Ex*>(realDevice) : nullptr;
    g_state.isEx         = isEx;
    g_state.logicalW     = logicalW;
    g_state.logicalH     = logicalH;
    StashTrackedBB(g_state);
    LeaveCriticalSection(&g_stateLock);

    static volatile LONG s_callbackRegistered = 0;
    if (InterlockedCompareExchange(&s_callbackRegistered, 1, 0) == 0)
        NvDirectMode::RegisterEyeChangeHandler(&OnEyeChange);

    return true;
}

} // namespace NvDirectMode
