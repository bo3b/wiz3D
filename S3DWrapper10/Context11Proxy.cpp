/* wiz3D - ID3D11DeviceContext proxy implementation (Option B Stage 2)
 *
 * Pure passthrough port of NvDirectMode/d3d11/Context11Proxy. The stage-3 BB
 * tracking and stage-4 magic-header capture were stripped for the MVP — the
 * job here is to prove COM identity + refcounting are right, not to do any
 * stereo work yet. OMSet/RSSetViewports/CopyResource/CopySubresourceRegion
 * are forwarded unchanged; per-eye behaviour will be re-added in Stage 4.
 */

#include "StdAfx.h"
#include "Context11Proxy.h"
#include "Device11Proxy.h"
#include "AdapterFunctions.h"  // DDILog

#pragma comment(lib, "dxguid.lib")

namespace wiz3d
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
    // Context1+ family: refuse so games fall back to the wrapped base
    // interface instead of getting an unwrapped escape hatch.
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

void STDMETHODCALLTYPE Context11Proxy::OMSetRenderTargets(
    UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews,
    ID3D11DepthStencilView* pDepthStencilView)
{
    m_real->OMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);
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
}

void STDMETHODCALLTYPE Context11Proxy::RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT* pViewports)
{
    m_real->RSSetViewports(NumViewports, pViewports);
}

void STDMETHODCALLTYPE Context11Proxy::CopyResource(
    ID3D11Resource* pDstResource, ID3D11Resource* pSrcResource)
{
    m_real->CopyResource(pDstResource, pSrcResource);
}

void STDMETHODCALLTYPE Context11Proxy::CopySubresourceRegion(
    ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY,
    UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource,
    const D3D11_BOX* pSrcBox)
{
    m_real->CopySubresourceRegion(pDstResource, DstSubresource, DstX, DstY, DstZ,
                                  pSrcResource, SrcSubresource, pSrcBox);
}

void STDMETHODCALLTYPE Context11Proxy::GetDevice(ID3D11Device** ppDevice)
{
    // COM identity: GetDevice must return the wrapped device, not the real
    // one — otherwise a game that round-trips through GetDevice ends up
    // bypassing our wrapper for subsequent resource creation.
    if (!ppDevice) return;
    if (m_parent)
    {
        // Device11Proxy publicly inherits from ID3D11Device — real upcast,
        // static_cast keeps /W4 + warnings-as-errors happy.
        *ppDevice = static_cast<ID3D11Device*>(m_parent);
        m_parent->AddRef();
        return;
    }
    m_real->GetDevice(ppDevice);
}

} // namespace wiz3d
