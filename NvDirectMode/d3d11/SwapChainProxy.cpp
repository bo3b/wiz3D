#include "SwapChainProxy.h"
#include "Device11Proxy.h"

#pragma comment(lib, "dxguid.lib")  // for IID_IDXGISwapChain et al

namespace NvDirectMode
{

SwapChainProxy::SwapChainProxy(IDXGISwapChain* real, Device11Proxy* parent)
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
    // SwapChain1/2/3 etc. — pass through to real for now.
    return m_real->QueryInterface(riid, ppvObj);
}

HRESULT STDMETHODCALLTYPE SwapChainProxy::GetBuffer(UINT Buffer, REFIID riid, void** ppSurface)
{
    HRESULT hr = m_real->GetBuffer(Buffer, riid, ppSurface);
    // 1b-iv: register the back-buffer (buffer 0) on the device proxy so
    // CreateRenderTargetView can identity-tag any RTV the game derives from
    // it. We store the raw pointer regardless of which interface the game
    // QI'd for — single-inheritance Texture2D->Resource->DeviceChild->IUnknown
    // means the address is stable across those interfaces.
    if (SUCCEEDED(hr) && Buffer == 0 && ppSurface && *ppSurface && m_parent)
    {
        m_parent->RegisterBackBufferTexture(*ppSurface);
    }
    return hr;
}

// NOTE: ResizeBuffers is a plain inline passthrough for now (in the header).
// 1b-iii's doubling-on-resize logic is intentionally deferred — most games
// don't resize the swap chain during runtime, and the inline-with-helper
// arrangement was triggering an MSVC parser issue when this TU's includes
// pulled SwapChainProxy.h after Device11Proxy.h. Revisit if a Direct Mode
// game proves to need windowed-resize support.

} // namespace NvDirectMode
