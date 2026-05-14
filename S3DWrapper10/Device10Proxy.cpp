/* wiz3D - ID3D10Device proxy implementation (Option B for DX10) */

#include "StdAfx.h"
#include "Device10Proxy.h"
#include "Texture2D10Proxy.h"
#include "RTV10Proxy.h"
#include "DSV10Proxy.h"
#include "Buffer10Proxy.h"
#include "StereoHeuristic.h"
#include "proxy_factory.h"     // IID_wiz3D_Device10Proxy + TryUnwrap_10 helpers

#pragma comment(lib, "dxguid.lib")

// Defensive cap on stack arrays used to unwrap CB / VB / RTV pointer arrays.
// D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT = 8, CB slots are 14; 16 covers it.
static constexpr UINT kMaxUnwrapArray = 16;

namespace wiz3d
{

// Forward UnwrapBuf10 helper — same shape as the DX11 UnwrapBuf in
// Context11Proxy.cpp. Passing a Buffer10Proxy directly to D3D10 would crash
// because the runtime doesn't understand our vtable past ID3D10Buffer.
static inline ID3D10Buffer* UnwrapBuf10(ID3D10Buffer* p)
{
    if (!p) return nullptr;
    if (auto* bp = TryUnwrapBuffer_10(static_cast<ID3D10Resource*>(p)))
        return bp->GetReal();
    return p;
}

// Eye-aware resource unwrap. Texture proxies pick left vs right based on
// the active eye; Buffer proxies have no eye distinction (single real).
static inline ID3D10Resource* UnwrapResForEye10(ID3D10Resource* p, bool pickRight)
{
    if (!p) return nullptr;
    if (auto* tex = TryUnwrapTexture2D_10(p))
    {
        ID3D10Resource* right = tex->GetRealRight();
        return (pickRight && right) ? right : static_cast<ID3D10Resource*>(tex->GetReal());
    }
    if (auto* buf = TryUnwrapBuffer_10(p))
        return static_cast<ID3D10Resource*>(buf->GetReal());
    return p;
}

Device10Proxy::Device10Proxy(ID3D10Device* real)
    : m_real(real)
    , m_refs(1)
    , m_activeEye(Eye::Left)
    , m_logicalWidth(0)
    , m_logicalHeight(0)
{
}

Device10Proxy::~Device10Proxy()
{
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
    // Private IID for cross-DLL identity (DXGIFactoryWrapper's Option B
    // factory hook QIs incoming pDevice to detect a Device10Proxy).
    if (riid == IID_wiz3D_Device10Proxy)
    {
        *ppvObj = static_cast<IUnknown*>(static_cast<ID3D10Device*>(this));
        AddRef();
        return S_OK;
    }
    // ID3D10Device1 / IDXGIDevice etc — pass through unwrapped for now.
    // Stage 3 of the DX10 port can extend this for COM identity preservation.
    return m_real->QueryInterface(riid, ppvObj);
}

// ---------------------------------------------------------------------------
// CreateXxx — wrap returned resources/views so downstream Try*Unwrap_10
// picks them up at the COM boundary.
// ---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE Device10Proxy::CreateBuffer(
    const D3D10_BUFFER_DESC* pDesc,
    const D3D10_SUBRESOURCE_DATA* pInitialData,
    ID3D10Buffer** ppBuffer)
{
    HRESULT hr = m_real->CreateBuffer(pDesc, pInitialData, ppBuffer);
    if (FAILED(hr) || !ppBuffer || !*ppBuffer) return hr;
    auto* bufProxy = new Buffer10Proxy(*ppBuffer, this);
    *ppBuffer = static_cast<ID3D10Buffer*>(bufProxy);
    return hr;
}

HRESULT STDMETHODCALLTYPE Device10Proxy::CreateTexture2D(
    const D3D10_TEXTURE2D_DESC* pDesc,
    const D3D10_SUBRESOURCE_DATA* pInitialData,
    ID3D10Texture2D** ppTexture2D)
{
    HRESULT hr = m_real->CreateTexture2D(pDesc, pInitialData, ppTexture2D);
    if (FAILED(hr) || !ppTexture2D || !*ppTexture2D) return hr;

    SIZE bbSize = { (LONG)m_logicalWidth, (LONG)m_logicalHeight };
    const SIZE* pBBSize = (m_logicalWidth > 0 && m_logicalHeight > 0) ? &bbSize : nullptr;

    ID3D10Texture2D* realLeft  = *ppTexture2D;
    ID3D10Texture2D* realRight = nullptr;
    if (ShouldDoubleTexture2D(pDesc, pBBSize))
    {
        HRESULT hr2 = m_real->CreateTexture2D(pDesc, nullptr, &realRight);
        if (FAILED(hr2)) realRight = nullptr;
    }

    auto* texProxy = new Texture2D10Proxy(realLeft, realRight, this);
    *ppTexture2D = static_cast<ID3D10Texture2D*>(texProxy);
    return hr;
}

HRESULT STDMETHODCALLTYPE Device10Proxy::CreateRenderTargetView(
    ID3D10Resource* pResource, const D3D10_RENDER_TARGET_VIEW_DESC* pDesc,
    ID3D10RenderTargetView** ppRTView)
{
    Texture2D10Proxy* texProxy  = TryUnwrapTexture2D_10(pResource);
    Buffer10Proxy*    bufProxy  = TryUnwrapBuffer_10(pResource);
    ID3D10Resource*   realLeft  = texProxy ? static_cast<ID3D10Resource*>(texProxy->GetReal())
                                : bufProxy ? static_cast<ID3D10Resource*>(bufProxy->GetReal())
                                           : pResource;
    ID3D10Resource*   realRight = texProxy ? static_cast<ID3D10Resource*>(texProxy->GetRealRight())
                                           : nullptr;

    HRESULT hr = m_real->CreateRenderTargetView(realLeft, pDesc, ppRTView);
    if (FAILED(hr) || !ppRTView || !*ppRTView) return hr;

    if (texProxy)
    {
        ID3D10RenderTargetView* realRightRTV = nullptr;
        if (realRight)
        {
            if (FAILED(m_real->CreateRenderTargetView(realRight, pDesc, &realRightRTV)))
                realRightRTV = nullptr;
        }
        auto* rtvProxy = new RTV10Proxy(*ppRTView, realRightRTV, this);
        *ppRTView = static_cast<ID3D10RenderTargetView*>(rtvProxy);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE Device10Proxy::CreateDepthStencilView(
    ID3D10Resource* pResource, const D3D10_DEPTH_STENCIL_VIEW_DESC* pDesc,
    ID3D10DepthStencilView** ppDepthStencilView)
{
    Texture2D10Proxy* texProxy  = TryUnwrapTexture2D_10(pResource);
    Buffer10Proxy*    bufProxy  = TryUnwrapBuffer_10(pResource);
    ID3D10Resource*   realLeft  = texProxy ? static_cast<ID3D10Resource*>(texProxy->GetReal())
                                : bufProxy ? static_cast<ID3D10Resource*>(bufProxy->GetReal())
                                           : pResource;
    ID3D10Resource*   realRight = texProxy ? static_cast<ID3D10Resource*>(texProxy->GetRealRight())
                                           : nullptr;

    HRESULT hr = m_real->CreateDepthStencilView(realLeft, pDesc, ppDepthStencilView);
    if (FAILED(hr) || !ppDepthStencilView || !*ppDepthStencilView) return hr;

    if (texProxy)
    {
        ID3D10DepthStencilView* realRightDSV = nullptr;
        if (realRight)
        {
            if (FAILED(m_real->CreateDepthStencilView(realRight, pDesc, &realRightDSV)))
                realRightDSV = nullptr;
        }
        auto* dsvProxy = new DSV10Proxy(*ppDepthStencilView, realRightDSV, this);
        *ppDepthStencilView = static_cast<ID3D10DepthStencilView*>(dsvProxy);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE Device10Proxy::CreateShaderResourceView(
    ID3D10Resource* pResource, const D3D10_SHADER_RESOURCE_VIEW_DESC* pDesc,
    ID3D10ShaderResourceView** ppSRView)
{
    Texture2D10Proxy* texProxy  = TryUnwrapTexture2D_10(pResource);
    Buffer10Proxy*    bufProxy  = TryUnwrapBuffer_10(pResource);
    ID3D10Resource*   realToUse = texProxy ? static_cast<ID3D10Resource*>(texProxy->GetReal())
                                : bufProxy ? static_cast<ID3D10Resource*>(bufProxy->GetReal())
                                           : pResource;
    return m_real->CreateShaderResourceView(realToUse, pDesc, ppSRView);
}

// ---------------------------------------------------------------------------
// *SetConstantBuffers — unwrap + VS-pipeline tag (Stage 4c.1 carryover).
// ---------------------------------------------------------------------------
#define D3D10_CB_SET(STAGE_PREFIX, IS_VS_PIPELINE)                                          \
void STDMETHODCALLTYPE Device10Proxy::STAGE_PREFIX##SetConstantBuffers(                     \
    UINT StartSlot, UINT NumBuffers, ID3D10Buffer* const* ppConstantBuffers)                \
{                                                                                           \
    ID3D10Buffer* raw[kMaxUnwrapArray] = { 0 };                                             \
    UINT cap = NumBuffers <= kMaxUnwrapArray ? NumBuffers : kMaxUnwrapArray;                \
    for (UINT i = 0; i < cap; ++i)                                                          \
    {                                                                                       \
        ID3D10Buffer* p = ppConstantBuffers ? ppConstantBuffers[i] : nullptr;               \
        if (p)                                                                              \
        {                                                                                   \
            if (auto* bp = TryUnwrapBuffer_10(static_cast<ID3D10Resource*>(p)))             \
            {                                                                               \
                if (IS_VS_PIPELINE) bp->TagVSBound();                                       \
                raw[i] = bp->GetReal();                                                     \
            }                                                                               \
            else                                                                            \
            {                                                                               \
                raw[i] = p;                                                                 \
            }                                                                               \
        }                                                                                   \
    }                                                                                       \
    m_real->STAGE_PREFIX##SetConstantBuffers(StartSlot, NumBuffers,                         \
        ppConstantBuffers ? raw : nullptr);                                                 \
}
D3D10_CB_SET(VS, 1)
D3D10_CB_SET(PS, 0)
D3D10_CB_SET(GS, 1)
#undef D3D10_CB_SET

void STDMETHODCALLTYPE Device10Proxy::IASetVertexBuffers(
    UINT StartSlot, UINT NumBuffers, ID3D10Buffer* const* ppVertexBuffers,
    const UINT* pStrides, const UINT* pOffsets)
{
    ID3D10Buffer* raw[kMaxUnwrapArray] = { 0 };
    UINT cap = NumBuffers <= kMaxUnwrapArray ? NumBuffers : kMaxUnwrapArray;
    for (UINT i = 0; i < cap; ++i)
        raw[i] = ppVertexBuffers ? UnwrapBuf10(ppVertexBuffers[i]) : nullptr;
    m_real->IASetVertexBuffers(StartSlot, NumBuffers,
        ppVertexBuffers ? raw : nullptr, pStrides, pOffsets);
}

void STDMETHODCALLTYPE Device10Proxy::IASetIndexBuffer(
    ID3D10Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset)
{
    m_real->IASetIndexBuffer(UnwrapBuf10(pIndexBuffer), Format, Offset);
}

void STDMETHODCALLTYPE Device10Proxy::SOSetTargets(
    UINT NumBuffers, ID3D10Buffer* const* ppSOTargets, const UINT* pOffsets)
{
    ID3D10Buffer* raw[kMaxUnwrapArray] = { 0 };
    UINT cap = NumBuffers <= kMaxUnwrapArray ? NumBuffers : kMaxUnwrapArray;
    for (UINT i = 0; i < cap; ++i)
        raw[i] = ppSOTargets ? UnwrapBuf10(ppSOTargets[i]) : nullptr;
    m_real->SOSetTargets(NumBuffers,
        ppSOTargets ? raw : nullptr, pOffsets);
}

// ---------------------------------------------------------------------------
// OMSetRenderTargets — eye-aware. Stage 1 of the port: m_activeEye starts
// Left and never changes (no replay sweep yet for DX10) so the right-eye
// siblings allocated by CreateTexture2D / CreateRenderTargetView are
// allocated but never bound. Stage 4 of the DX10 port will toggle the eye.
// ---------------------------------------------------------------------------
void STDMETHODCALLTYPE Device10Proxy::OMSetRenderTargets(
    UINT NumViews, ID3D10RenderTargetView* const* ppRenderTargetViews,
    ID3D10DepthStencilView* pDepthStencilView)
{
    bool pickRight = (m_activeEye == Eye::Right);
    ID3D10RenderTargetView* rawRTVs[kMaxUnwrapArray] = { 0 };
    ID3D10RenderTargetView* const* rtvsToUse = ppRenderTargetViews;
    if (NumViews > 0 && ppRenderTargetViews)
    {
        UINT cap = NumViews <= kMaxUnwrapArray ? NumViews : kMaxUnwrapArray;
        for (UINT i = 0; i < cap; ++i)
        {
            RTV10Proxy* p = TryUnwrapRTV_10(ppRenderTargetViews[i]);
            if (!p) { rawRTVs[i] = ppRenderTargetViews[i]; continue; }
            ID3D10RenderTargetView* right = p->GetRealRight();
            rawRTVs[i] = (pickRight && right) ? right : p->GetReal();
        }
        rtvsToUse = rawRTVs;
    }
    ID3D10DepthStencilView* realDSV = pDepthStencilView;
    if (DSV10Proxy* d = TryUnwrapDSV_10(pDepthStencilView))
    {
        ID3D10DepthStencilView* right = d->GetRealRight();
        realDSV = (pickRight && right) ? right : d->GetReal();
    }
    m_real->OMSetRenderTargets(NumViews, rtvsToUse, realDSV);
}

void STDMETHODCALLTYPE Device10Proxy::CopyResource(
    ID3D10Resource* pDstResource, ID3D10Resource* pSrcResource)
{
    bool pickRight = (m_activeEye == Eye::Right);
    m_real->CopyResource(UnwrapResForEye10(pDstResource, pickRight),
                         UnwrapResForEye10(pSrcResource, pickRight));
}

void STDMETHODCALLTYPE Device10Proxy::CopySubresourceRegion(
    ID3D10Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY,
    UINT DstZ, ID3D10Resource* pSrcResource, UINT SrcSubresource,
    const D3D10_BOX* pSrcBox)
{
    bool pickRight = (m_activeEye == Eye::Right);
    m_real->CopySubresourceRegion(
        UnwrapResForEye10(pDstResource, pickRight), DstSubresource, DstX, DstY, DstZ,
        UnwrapResForEye10(pSrcResource, pickRight), SrcSubresource, pSrcBox);
}

void STDMETHODCALLTYPE Device10Proxy::UpdateSubresource(
    ID3D10Resource* pDstResource, UINT DstSubresource, const D3D10_BOX* pDstBox,
    const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
    bool pickRight = (m_activeEye == Eye::Right);
    m_real->UpdateSubresource(UnwrapResForEye10(pDstResource, pickRight),
                              DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}

void STDMETHODCALLTYPE Device10Proxy::ResolveSubresource(
    ID3D10Resource* pDstResource, UINT DstSubresource,
    ID3D10Resource* pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format)
{
    bool pickRight = (m_activeEye == Eye::Right);
    m_real->ResolveSubresource(
        UnwrapResForEye10(pDstResource, pickRight), DstSubresource,
        UnwrapResForEye10(pSrcResource, pickRight), SrcSubresource, Format);
}

void STDMETHODCALLTYPE Device10Proxy::ClearRenderTargetView(
    ID3D10RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4])
{
    bool pickRight = (m_activeEye == Eye::Right);
    ID3D10RenderTargetView* real = pRenderTargetView;
    if (auto* rtv = TryUnwrapRTV_10(pRenderTargetView))
    {
        ID3D10RenderTargetView* right = rtv->GetRealRight();
        real = (pickRight && right) ? right : rtv->GetReal();
    }
    m_real->ClearRenderTargetView(real, ColorRGBA);
}

void STDMETHODCALLTYPE Device10Proxy::ClearDepthStencilView(
    ID3D10DepthStencilView* pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
    bool pickRight = (m_activeEye == Eye::Right);
    ID3D10DepthStencilView* real = pDepthStencilView;
    if (auto* dsv = TryUnwrapDSV_10(pDepthStencilView))
    {
        ID3D10DepthStencilView* right = dsv->GetRealRight();
        real = (pickRight && right) ? right : dsv->GetReal();
    }
    m_real->ClearDepthStencilView(real, ClearFlags, Depth, Stencil);
}

} // namespace wiz3d
