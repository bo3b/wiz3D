#include "SwapChainProxy.h"
#include "Device10Proxy.h"

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
    return m_real->QueryInterface(riid, ppvObj);
}

HRESULT STDMETHODCALLTYPE SwapChainProxy::GetBuffer(UINT Buffer, REFIID riid, void** ppSurface)
{
    HRESULT hr = m_real->GetBuffer(Buffer, riid, ppSurface);
    // 1b-iv: register the back buffer (slot 0) on the device proxy so
    // CreateRenderTargetView can identity-tag any RTV the game derives.
    if (SUCCEEDED(hr) && Buffer == 0 && ppSurface && *ppSurface && m_parent)
    {
        m_parent->RegisterBackBufferTexture(*ppSurface);
    }
    return hr;
}

} // namespace NvDirectMode
