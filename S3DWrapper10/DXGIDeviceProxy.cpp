#include "StdAfx.h"
#include "DXGIDeviceProxy.h"
#include "Device11Proxy.h"
#include "AdapterFunctions.h"   // DDILog

#include <d3d11.h>

#pragma comment(lib, "dxguid.lib")

#define LOG_VERBOSE(fmt, ...)         DDILog(fmt, ##__VA_ARGS__)
#define NVDM_TRACE_FIRST_N(n, fmt, ...) do { static int s_n = 0; if (s_n < (n)) { DDILog(fmt, ##__VA_ARGS__); ++s_n; } } while(0)

namespace wiz3d
{

DXGIDeviceProxy::DXGIDeviceProxy(Device11Proxy* parent,
                                 IDXGIDevice*  r0,
                                 IDXGIDevice1* r1,
                                 IDXGIDevice2* r2,
                                 IDXGIDevice3* r3)
    : m_parent(parent)
    , m_real0(r0), m_real1(r1), m_real2(r2), m_real3(r3)
    , m_highestVer(r3 ? 3 : r2 ? 2 : r1 ? 1 : 0)
    , m_refs(1)
{
    LOG_VERBOSE("  DXGIDeviceProxy ctor: parent=%p real0=%p real1=%p real2=%p real3=%p (highestVer=%d)\n",
                parent, r0, r1, r2, r3, m_highestVer);
}

DXGIDeviceProxy::~DXGIDeviceProxy()
{
    // Each non-null pointer was obtained via QueryInterface on the real
    // device family — independent refs that all have to be released.
    if (m_real3) { m_real3->Release(); m_real3 = nullptr; }
    if (m_real2) { m_real2->Release(); m_real2 = nullptr; }
    if (m_real1) { m_real1->Release(); m_real1 = nullptr; }
    if (m_real0) { m_real0->Release(); m_real0 = nullptr; }
}

ULONG STDMETHODCALLTYPE DXGIDeviceProxy::Release()
{
    LONG r = InterlockedDecrement(&m_refs);
    if (r == 0) delete this;
    return (ULONG)r;
}

HRESULT STDMETHODCALLTYPE DXGIDeviceProxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;

    // The COM-identity loop fix: if the game asks for the underlying
    // ID3D11Device family, return our Device11Proxy so subsequent calls
    // route through the wrapper.
    if (riid == __uuidof(ID3D11Device))
    {
        if (!m_parent) return E_NOINTERFACE;
        return m_parent->QueryInterface(riid, ppvObj);
    }

    // Self-claim our own interface family.
    if (riid == IID_IUnknown ||
        riid == IID_IDXGIObject ||
        riid == IID_IDXGIDeviceSubObject ||
        riid == IID_IDXGIDevice)
    {
        *ppvObj = static_cast<IDXGIDevice*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_IDXGIDevice1 && m_real1)
    {
        *ppvObj = static_cast<IDXGIDevice1*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_IDXGIDevice2 && m_real2)
    {
        *ppvObj = static_cast<IDXGIDevice2*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_IDXGIDevice3 && m_real3)
    {
        *ppvObj = static_cast<IDXGIDevice3*>(this);
        AddRef();
        return S_OK;
    }

    // Anything else (IDXGIDevice4, ID3D11Device1+, vendor IIDs) — refuse so
    // games can't escape through the DXGI side either. Returning the raw real
    // device for these IIDs let games bypass our wrap on the resource-creation
    // path; per-frame trace confirmed `rtv0={proxy=00000000}` consistently.
    NVDM_TRACE_FIRST_N(8,
        "  DXGIDeviceProxy::QI(unhandled IID) -- refusing to avoid bypass\n");
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

// GetAdapter / GetParent(IDXGIAdapter*) used to wrap returned adapters in
// DXGIAdapterProxy so the game's adapter→GetParent walk would land on our
// DXGIFactoryProxy. After tasks #66 + #70 the real adapter's GetDesc /
// GetDesc1 vtable slots are spoofed for vendor and the real factory's
// CreateSwapChain* slots are hooked — the entire DXGI walk is now
// layout-stable on real pointers, no class wrap needed. Crucially this
// fixes Hitman Absolution + EGO engine games (Dirt Rally, Dirt 3, Dirt
// Showdown, Grid Autosport — all confirmed Direct Mode), whose engines
// crash when they walk struct internals past the wrapper's vtable.

HRESULT STDMETHODCALLTYPE DXGIDeviceProxy::GetAdapter(IDXGIAdapter** ppAdapter)
{
    HRESULT hr = m_real0->GetAdapter(ppAdapter);
    NVDM_TRACE_FIRST_N(4, "  DXGIDeviceProxy::GetAdapter: real adapter=%p (vtable-patched)\n",
                       ppAdapter ? (void*)*ppAdapter : nullptr);
    return hr;
}

HRESULT STDMETHODCALLTYPE DXGIDeviceProxy::GetParent(REFIID riid, void** ppParent)
{
    if (!ppParent) return E_POINTER;
    return m_real0->GetParent(riid, ppParent);
}

} // namespace wiz3d
