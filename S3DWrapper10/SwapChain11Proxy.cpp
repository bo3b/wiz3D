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
    DDILog("SwapChain11Proxy ctor: real=%p real1=%p parent=%p\n", real, real1, parent);
}

SwapChain11Proxy::~SwapChain11Proxy()
{
    if (m_real1) { m_real1->Release(); m_real1 = nullptr; }
    if (m_real)  { m_real->Release();  m_real  = nullptr; }
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

void SwapChain11Proxy::OnPresentBoundary()
{
    // Frame boundary trigger. Stage 4b.4: signal to the context that frame
    // boundaries actually fire here, so its state-setting methods may
    // safely record themselves (the recording vector will be flushed each
    // frame). For games whose swap chain bypasses us (factory two-call
    // path), this never fires and the context's recording stays disabled.
    //
    // Stage 4b.8 will add the actual left-then-right replay sweep before
    // forwarding to real Present; 4d will composite the doubled BB-RTVs
    // into the swap-chain BB. For now we just clear and signal active.
    if (!m_parent) return;
    Context11Proxy* ctx = m_parent->GetContextProxy();
    if (!ctx) return;
    ctx->SetPresentHookActive(true);
    ctx->ClearFrameCommands();
}

HRESULT STDMETHODCALLTYPE SwapChain11Proxy::Present(UINT SyncInterval, UINT Flags)
{
    OnPresentBoundary();
    return m_real->Present(SyncInterval, Flags);
}

HRESULT STDMETHODCALLTYPE SwapChain11Proxy::Present1(
    UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
    OnPresentBoundary();
    return m_real1 ? m_real1->Present1(SyncInterval, PresentFlags, pPresentParameters)
                   : E_NOINTERFACE;
}

} // namespace wiz3d
