/* wiz3D - IDXGISwapChain proxy implementation (Option B Stage 4b.2) */

#include "StdAfx.h"
#include "SwapChain11Proxy.h"
#include "Device11Proxy.h"
#include "Context11Proxy.h"
#include "proxy_factory.h"     // IID_wiz3D_SwapChain11Proxy (declared in 4b.2 update)
#include "AdapterFunctions.h"  // DDILog

#pragma comment(lib, "dxguid.lib")

namespace wiz3d
{

SwapChain11Proxy::SwapChain11Proxy(IDXGISwapChain* real, IDXGISwapChain1* real1, Device11Proxy* parent)
    : m_real(real)
    , m_real1(real1)
    , m_parent(parent)
    , m_refs(1)
{
    // Stage 4b.8 fix: AddRef the parent device. Without this, if the game
    // releases its Device11Proxy reference before the swap chain proxy,
    // m_parent dangles and the next Present hook reads garbage (BioShock
    // crashed at OnPresentBoundaryPre line 92 with ECX=1 — Device11Proxy
    // memory had been freed and m_ctxProxy was reading reused heap data).
    if (m_parent) m_parent->AddRef();
    DDILog("SwapChain11Proxy ctor: real=%p real1=%p parent=%p\n", real, real1, parent);
}

SwapChain11Proxy::~SwapChain11Proxy()
{
    if (m_real1)  { m_real1->Release();  m_real1  = nullptr; }
    if (m_real)   { m_real->Release();   m_real   = nullptr; }
    if (m_parent) { m_parent->Release(); m_parent = nullptr; }
}

ULONG STDMETHODCALLTYPE SwapChain11Proxy::Release()
{
    LONG r = InterlockedDecrement(&m_refs);
    if (r == 0) delete this;
    return (ULONG)r;
}

HRESULT STDMETHODCALLTYPE SwapChain11Proxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown          ||
        riid == IID_IDXGIObject       ||
        riid == IID_IDXGIDeviceSubObject ||
        riid == IID_IDXGISwapChain)
    {
        *ppvObj = static_cast<IDXGISwapChain*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_IDXGISwapChain1 && m_real1)
    {
        *ppvObj = static_cast<IDXGISwapChain1*>(this);
        AddRef();
        return S_OK;
    }
    // Stage 4b.2: private identity IID — used by the dxgi.dll-side factory
    // hook (4b.3) to detect "is this swap chain one of ours?" cross-DLL.
    if (riid == IID_wiz3D_SwapChain11Proxy)
    {
        *ppvObj = static_cast<IUnknown*>(static_cast<IDXGISwapChain*>(this));
        AddRef();
        return S_OK;
    }
    // IDXGISwapChain2/3/4: pass through unwrapped for now. Future iteration
    // can extend if needed by Win11-era games.
    return m_real->QueryInterface(riid, ppvObj);
}

HRESULT STDMETHODCALLTYPE SwapChain11Proxy::GetDevice(REFIID riid, void** ppDevice)
{
    // COM identity: route ID3D11Device QI through our wrapped device, so a
    // game that fetches the device via sc->GetDevice keeps using our proxy.
    if (!ppDevice) return E_POINTER;
    if (riid == __uuidof(ID3D11Device) && m_parent)
    {
        return m_parent->QueryInterface(riid, ppDevice);
    }
    return m_real->GetDevice(riid, ppDevice);
}

void SwapChain11Proxy::OnPresentBoundaryPre()
{
    // Stage 4b.8: PRE-PRESENT sweep. The game has already issued frame N's
    // state setters and draws — those ran for the left eye via the direct
    // passthrough in each proxy method. When UseCOMWrapReplay=1 we re-issue
    // the recorded command stream with m_activeEye=Right so OMSet/Clear/
    // Update/Copy/Resolve helpers bind right-eye real handles. After this,
    // the left-eye and right-eye textures both hold their per-eye images,
    // ready for the 4d SBS composite to flatten into the swap-chain BB.
    //
    // Gated behind UseCOMWrapReplay (default off) because:
    //   - Recorded handles can dangle across ResizeBuffers / SetFullscreen
    //     (Max Payne 3 fullscreen toggle crashes inside d3d11.dll during
    //     replay because BB-derived RTVs have been invalidated).
    //   - Right-eye textures aren't displayed yet (no 4c stereo math, no
    //     4d composite) so the replay has no visible benefit, only risk.
    if (!gInfo.UseCOMWrapReplay) return;
    if (!m_parent) return;
    Context11Proxy* ctx = m_parent->GetContextProxy();
    if (!ctx) return;
    if (ctx->IsPresentHookActive())
        ctx->ReplayFrameCommands(Context11Proxy::Eye::Right);
}

void SwapChain11Proxy::OnPresentBoundaryPost()
{
    // Stage 4b.8: POST-PRESENT housekeeping. Real Present has flipped the
    // BB; clear the recording vector and arm it for frame N+1. We arm
    // unconditionally (not just on the first call) so a context that's
    // been ClearState()'d still gets recording.
    if (!m_parent) return;
    Context11Proxy* ctx = m_parent->GetContextProxy();
    if (!ctx) return;
    ctx->ClearFrameCommands();
    ctx->SetPresentHookActive(true);
}

HRESULT STDMETHODCALLTYPE SwapChain11Proxy::ResizeBuffers(
    UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    // Stage 4b.8 fix: ResizeBuffers invalidates every BB-derived RTV/DSV
    // the game (and our recorded closures) might hold. The recorded
    // command stream becomes unsafe to replay — Max Payne 3's fullscreen
    // toggle hit this and crashed inside d3d11.dll when replay tried to
    // re-bind dead handles. Clear the recording before forwarding.
    if (m_parent)
    {
        if (Context11Proxy* ctx = m_parent->GetContextProxy())
            ctx->ClearFrameCommands();
    }
    return m_real->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
}

HRESULT STDMETHODCALLTYPE SwapChain11Proxy::SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget)
{
    // Stage 4b.8 fix: same logic as ResizeBuffers — fullscreen transitions
    // can implicitly resize the swap chain and invalidate BB-derived
    // handles. Clear the recording so replay can't re-issue stale state.
    if (m_parent)
    {
        if (Context11Proxy* ctx = m_parent->GetContextProxy())
            ctx->ClearFrameCommands();
    }
    return m_real->SetFullscreenState(Fullscreen, pTarget);
}

HRESULT STDMETHODCALLTYPE SwapChain11Proxy::Present(UINT SyncInterval, UINT Flags)
{
    OnPresentBoundaryPre();
    HRESULT hr = m_real->Present(SyncInterval, Flags);
    OnPresentBoundaryPost();
    return hr;
}

HRESULT STDMETHODCALLTYPE SwapChain11Proxy::Present1(
    UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
    OnPresentBoundaryPre();
    HRESULT hr = m_real1 ? m_real1->Present1(SyncInterval, PresentFlags, pPresentParameters)
                          : E_NOINTERFACE;
    OnPresentBoundaryPost();
    return hr;
}

} // namespace wiz3d
