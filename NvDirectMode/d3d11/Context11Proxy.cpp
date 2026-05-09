#include "Context11Proxy.h"
#include "Device11Proxy.h"
#include "eye_state.h"
#include "log.h"

#pragma comment(lib, "dxguid.lib")

namespace NvDirectMode
{

Context11Proxy::Context11Proxy(ID3D11DeviceContext* real, Device11Proxy* parent)
    : m_real(real)
    , m_parent(parent)
    , m_refs(1)
    , m_currentBBBound(false)
{
}

Context11Proxy::~Context11Proxy() = default;

HRESULT STDMETHODCALLTYPE Context11Proxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_ID3D11DeviceChild ||
        riid == IID_ID3D11DeviceContext)
    {
        *ppvObj = static_cast<ID3D11DeviceContext*>(this);
        AddRef();
        return S_OK;
    }
    // Context1/Context2/Context3+ — return E_NOINTERFACE so the game falls
    // back to the base ID3D11DeviceContext (which we wrap). Passing the
    // real Context1+ pointer through opens a COM-identity escape: the game
    // would call OMSetRenderTargets on the unwrapped pointer, bypassing our
    // active-eye viewport clamp. Tomb Raider 2013 was leaking 4 unwrapped
    // Context1+ refs per device and crashing in ucrtbase free() shortly
    // after first frame's OMSet — likely the game's Context1+ refcount
    // diverging from our Context11Proxy's tracking.
    NVDM_TRACE_FIRST_N(8,
        "  Context11Proxy::QI(unknown/higher IID) -> E_NOINTERFACE\n");
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

// 1b-iv core + stage-3 RSSetViewports re-clamp: when game binds the wrapped
// backbuffer RTV at slot 0, clamp the viewport to the active eye's half of
// the doubled shadow texture. Re-applied on game-issued RSSetViewports too,
// so a game that calls OMSet then RSSetViewports(fullscreen) doesn't
// overwrite our clamp.
namespace
{
    // Pulls the active eye, applies config swap, returns 1=RIGHT 2=LEFT 3=MONO.
    int ResolveActiveEye()
    {
        int eye = NvDirectMode::GetActiveEye();
        if (NvDM_SwapEyes())
        {
            if      (eye == NvDirectMode::kEyeLeft)  eye = NvDirectMode::kEyeRight;
            else if (eye == NvDirectMode::kEyeRight) eye = NvDirectMode::kEyeLeft;
        }
        return eye;
    }

    // Applies the per-eye viewport clamp. Pulls logical W/H off the parent
    // device. Returns true if it issued an RSSetViewports.
    bool SetEyeViewport(ID3D11DeviceContext* ctx, Device11Proxy* parent,
                        ID3D11RenderTargetView* rtv, const char* tag)
    {
        if (!parent || !ctx) return false;
        UINT logicalW = parent->GetLogicalBackBufferWidth();
        UINT logicalH = parent->GetLogicalBackBufferHeight();
        if (logicalW == 0 || logicalH == 0) return false;

        int eye = ResolveActiveEye();
        const bool topBottom = NvDM_OutputIsTopBottom() != 0;
        D3D11_VIEWPORT vp;
        vp.TopLeftX = (!topBottom && eye == NvDirectMode::kEyeRight) ? (FLOAT)logicalW : 0.0f;
        vp.TopLeftY = ( topBottom && eye == NvDirectMode::kEyeRight) ? (FLOAT)logicalH : 0.0f;
        vp.Width    = (FLOAT)logicalW;
        vp.Height   = (FLOAT)logicalH;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
        NVDM_TRACE_FIRST_N(16, "  Context11Proxy::%s eye=%d mode=%s viewport=(%.0f,%.0f %.0fx%.0f) rtv=%p\n",
                           tag, eye, topBottom ? "T-B" : "SBS",
                           vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height, rtv);
        return true;
    }

    bool ApplyEyeViewportIfBackBufferBound(ID3D11DeviceContext* ctx, Device11Proxy* parent,
                                           UINT NumViews, ID3D11RenderTargetView* const* rtvs,
                                           const char* tag)
    {
        if (!parent || NumViews == 0 || !rtvs || !rtvs[0]) return false;
        if (!parent->IsBackBufferRTV(rtvs[0])) return false;
        return SetEyeViewport(ctx, parent, rtvs[0], tag);
    }
}

void STDMETHODCALLTYPE Context11Proxy::OMSetRenderTargets(
    UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews,
    ID3D11DepthStencilView* pDepthStencilView)
{
    m_real->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
    bool bbBound = ApplyEyeViewportIfBackBufferBound(m_real, m_parent,
                                                      NumViews, ppRenderTargetViews, "OMSet");
    m_currentBBBound = bbBound;
}

void STDMETHODCALLTYPE Context11Proxy::OMSetRenderTargetsAndUnorderedAccessViews(
    UINT NumRTVs, ID3D11RenderTargetView* const* ppRenderTargetViews,
    ID3D11DepthStencilView* pDepthStencilView,
    UINT UAVStartSlot, UINT NumUAVs,
    ID3D11UnorderedAccessView* const* ppUnorderedAccessViews,
    const UINT* pUAVInitialCounts)
{
    m_real->OMSetRenderTargetsAndUnorderedAccessViews(
        NumRTVs, ppRenderTargetViews, pDepthStencilView,
        UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
    // D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL == (UINT)-1 — when game
    // uses that sentinel we skip our hook (RTVs unchanged from prior bind).
    if (NumRTVs != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL)
    {
        bool bbBound = ApplyEyeViewportIfBackBufferBound(m_real, m_parent,
                                                          NumRTVs, ppRenderTargetViews, "OMSetUAV");
        m_currentBBBound = bbBound;
    }
}

// Stage 3 RSSetViewports hook: if the game-bound RTV at slot 0 is our BB
// shadow, force the viewport to the active eye's half regardless of what
// the game requested. Without this, a game that calls
//     OMSetRenderTargets(BB, ...)    // we set per-eye viewport
//     RSSetViewports(0,0,W,H)        // game overrides — both eyes get same render
// loses per-eye routing for that frame. With shadow-RT the game thinks the
// BB is one-eye-sized and would try to set a viewport covering the whole
// "screen", so this re-clamp is the difference between stereo working and
// the game drawing the same content into both halves.
void STDMETHODCALLTYPE Context11Proxy::RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT* pViewports)
{
    m_real->RSSetViewports(NumViewports, pViewports);
    if (m_currentBBBound)
        SetEyeViewport(m_real, m_parent, nullptr, "RSSetViewports-reclamp");
}

void STDMETHODCALLTYPE Context11Proxy::GetDevice(ID3D11Device** ppDevice)
{
    // COM identity: GetDevice on the wrapped context should return the
    // wrapped device, not the unwrapped real one — otherwise a game that
    // round-trips through GetDevice ends up holding two different "device"
    // pointers and our wrapper gets bypassed for resource creation.
    if (!ppDevice) return;
    if (m_parent)
    {
        *ppDevice = reinterpret_cast<ID3D11Device*>(m_parent);
        m_parent->AddRef();
        return;
    }
    m_real->GetDevice(ppDevice);
}

} // namespace NvDirectMode
