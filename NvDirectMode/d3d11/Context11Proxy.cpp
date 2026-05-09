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
    // Context1/Context2/Context3 etc — pass through unwrapped for now.
    HRESULT hr = m_real->QueryInterface(riid, ppvObj);
    NVDM_TRACE_FIRST_N(8,
        "  Context11Proxy::QI(unknown IID, e.g. Context1+) hr=0x%08lX -- bypass risk\n", hr);
    return hr;
}

// 1b-iv core: when game binds the wrapped backbuffer RTV at slot 0, clamp
// the viewport to the active eye's half. Game-set viewports later still
// pass through unchanged (we just override the default-after-OMSet that
// otherwise covers the full doubled 2W width).
namespace
{
    void ApplyEyeViewportIfBackBufferBound(ID3D11DeviceContext* ctx, Device11Proxy* parent,
                                           UINT NumViews, ID3D11RenderTargetView* const* rtvs)
    {
        if (!parent || NumViews == 0 || !rtvs) return;
        // We only route on slot 0 — the conventional backbuffer RTV slot.
        // Multi-RT renders to backbuffer + auxiliary targets land here too;
        // game's draw covers slot 0 with our chosen viewport regardless.
        if (!parent->IsBackBufferRTV(rtvs[0])) return;

        UINT logicalW = parent->GetLogicalBackBufferWidth();
        UINT logicalH = parent->GetLogicalBackBufferHeight();
        if (logicalW == 0 || logicalH == 0) return;

        int eye = NvDirectMode::GetActiveEye();
        // Apply config swap eyes — flip routing of LEFT and RIGHT.
        if (NvDM_SwapEyes())
        {
            if      (eye == NvDirectMode::kEyeLeft)  eye = NvDirectMode::kEyeRight;
            else if (eye == NvDirectMode::kEyeRight) eye = NvDirectMode::kEyeLeft;
        }
        D3D11_VIEWPORT vp;
        vp.TopLeftX = (eye == NvDirectMode::kEyeRight) ? (FLOAT)logicalW : 0.0f;
        vp.TopLeftY = 0.0f;
        vp.Width    = (FLOAT)logicalW;
        vp.Height   = (FLOAT)logicalH;
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        ctx->RSSetViewports(1, &vp);
        NVDM_TRACE_FIRST_N(16, "  Context11Proxy::OMSet eye=%d viewport=(%.0f,%.0f %.0fx%.0f) rtv=%p\n",
                           eye, vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height, rtvs[0]);
    }
}

void STDMETHODCALLTYPE Context11Proxy::OMSetRenderTargets(
    UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews,
    ID3D11DepthStencilView* pDepthStencilView)
{
    m_real->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
    ApplyEyeViewportIfBackBufferBound(m_real, m_parent, NumViews, ppRenderTargetViews);
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
        ApplyEyeViewportIfBackBufferBound(m_real, m_parent, NumRTVs, ppRenderTargetViews);
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
