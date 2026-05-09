#include "SwapChainProxy.h"
#include "Device10Proxy.h"
#include "log.h"

#pragma comment(lib, "dxguid.lib")

namespace NvDirectMode
{

SwapChainProxy::SwapChainProxy(IDXGISwapChain* real, Device10Proxy* parent)
    : m_real(real)
    , m_parent(parent)
    , m_refs(1)
{
}

SwapChainProxy::~SwapChainProxy() = default;

HRESULT STDMETHODCALLTYPE SwapChainProxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDXGIObject ||
        riid == IID_IDXGIDeviceSubObject || riid == IID_IDXGISwapChain)
    {
        *ppvObj = static_cast<IDXGISwapChain*>(this);
        AddRef();
        return S_OK;
    }
    HRESULT hr = m_real->QueryInterface(riid, ppvObj);
    NVDM_TRACE_FIRST_N(8, "  d3d10/SwapChainProxy::QI(unknown IID) hr=0x%08lX -- bypass risk\n", hr);
    return hr;
}

HRESULT STDMETHODCALLTYPE SwapChainProxy::GetBuffer(UINT Buffer, REFIID riid, void** ppSurface)
{
    HRESULT hr = m_real->GetBuffer(Buffer, riid, ppSurface);
    if (SUCCEEDED(hr) && Buffer == 0 && ppSurface && *ppSurface && m_parent)
    {
        m_parent->RegisterBackBufferTexture(*ppSurface);
        LOG_VERBOSE("  d3d10/SwapChainProxy::GetBuffer(0): registered BB texture=%p on parent=%p\n",
                    *ppSurface, m_parent);
    }
    return hr;
}

} // namespace NvDirectMode
