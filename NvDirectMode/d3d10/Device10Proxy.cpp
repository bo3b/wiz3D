#include "Device10Proxy.h"
#include "eye_state.h"

#pragma comment(lib, "dxguid.lib")

namespace NvDirectMode
{

Device10Proxy::Device10Proxy(ID3D10Device* real)
    : m_real(real)
    , m_refs(1)
    , m_logicalWidth(0)
    , m_logicalHeight(0)
    , m_pBackBufferResource(nullptr)
{
    InitializeCriticalSection(&m_rtvSetLock);
}

Device10Proxy::~Device10Proxy()
{
    DeleteCriticalSection(&m_rtvSetLock);
}

HRESULT STDMETHODCALLTYPE Device10Proxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_ID3D10Device)
    {
        *ppvObj = static_cast<ID3D10Device*>(this);
        AddRef();
        return S_OK;
    }
    // ID3D10Device1 — pass through unwrapped for now (Device1 wrapping deferred).
    return m_real->QueryInterface(riid, ppvObj);
}

void Device10Proxy::RegisterBackBufferTexture(void* pTextureLike)
{
    m_pBackBufferResource = pTextureLike;
}

bool Device10Proxy::IsBackBufferResource(ID3D10Resource* p) const
{
    return p && static_cast<void*>(p) == m_pBackBufferResource;
}

void Device10Proxy::TrackBackBufferRTV(ID3D10RenderTargetView* rtv)
{
    if (!rtv) return;
    EnterCriticalSection(&m_rtvSetLock);
    m_backBufferRTVs.insert(rtv);
    LeaveCriticalSection(&m_rtvSetLock);
}

bool Device10Proxy::IsBackBufferRTV(ID3D10RenderTargetView* rtv) const
{
    if (!rtv) return false;
    auto* self = const_cast<Device10Proxy*>(this);
    EnterCriticalSection(&self->m_rtvSetLock);
    bool found = m_backBufferRTVs.find(rtv) != m_backBufferRTVs.end();
    LeaveCriticalSection(&self->m_rtvSetLock);
    return found;
}

HRESULT STDMETHODCALLTYPE Device10Proxy::CreateRenderTargetView(
    ID3D10Resource* pResource, const D3D10_RENDER_TARGET_VIEW_DESC* pDesc,
    ID3D10RenderTargetView** ppRTView)
{
    HRESULT hr = m_real->CreateRenderTargetView(pResource, pDesc, ppRTView);
    if (SUCCEEDED(hr) && ppRTView && *ppRTView && IsBackBufferResource(pResource))
    {
        TrackBackBufferRTV(*ppRTView);
    }
    return hr;
}

void STDMETHODCALLTYPE Device10Proxy::OMSetRenderTargets(
    UINT NumViews, ID3D10RenderTargetView* const* ppRenderTargetViews,
    ID3D10DepthStencilView* pDepthStencilView)
{
    m_real->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
    if (NumViews == 0 || !ppRenderTargetViews || !IsBackBufferRTV(ppRenderTargetViews[0])) return;
    if (m_logicalWidth == 0 || m_logicalHeight == 0) return;

    const int eye = GetActiveEye();
    D3D10_VIEWPORT vp;
    vp.TopLeftX = (eye == kEyeRight) ? (INT)m_logicalWidth : 0;
    vp.TopLeftY = 0;
    vp.Width    = m_logicalWidth;
    vp.Height   = m_logicalHeight;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    m_real->RSSetViewports(1, &vp);
}

} // namespace NvDirectMode
