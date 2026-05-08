/* NvDirectMode - IDirect3D9 proxy implementation */

#include "D3D9Proxy.h"
#include "Device9Proxy.h"
#include "proxy_factory.h"
#include "swapchain_helpers.h"

#pragma comment(lib, "dxguid.lib")  // for IID_IDirect3D9 / IID_IDirect3DDevice9

namespace NvDirectMode
{

void* CreateD3D9Proxy(void* realD3D9)
{
    if (!realD3D9) return nullptr;
    auto* p = new D3D9Proxy(static_cast<IDirect3D9*>(realD3D9), /*isEx=*/false);
    return static_cast<IDirect3D9*>(p);
}

void* CreateD3D9ExProxy(void* realD3D9Ex)
{
    if (!realD3D9Ex) return nullptr;
    auto* p = new D3D9Proxy(static_cast<IDirect3D9Ex*>(realD3D9Ex), /*isEx=*/true);
    return static_cast<IDirect3D9Ex*>(p);
}


D3D9Proxy::D3D9Proxy(IDirect3D9* real, bool isEx)
    : m_real(real)
    , m_realEx(isEx ? static_cast<IDirect3D9Ex*>(real) : nullptr)
    , m_isEx(isEx)
    , m_refs(1)
{
}

D3D9Proxy::~D3D9Proxy() = default;

HRESULT STDMETHODCALLTYPE D3D9Proxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirect3D9)
    {
        *ppvObj = static_cast<IDirect3D9*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_IDirect3D9Ex)
    {
        if (!m_isEx) { *ppvObj = nullptr; return E_NOINTERFACE; }
        *ppvObj = static_cast<IDirect3D9Ex*>(this);
        AddRef();
        return S_OK;
    }
    return m_real->QueryInterface(riid, ppvObj);
}

ULONG STDMETHODCALLTYPE D3D9Proxy::AddRef()  { return InterlockedIncrement(&m_refs); }
ULONG STDMETHODCALLTYPE D3D9Proxy::Release()
{
    LONG r = InterlockedDecrement(&m_refs);
    if (r == 0)
    {
        if (m_real) { m_real->Release(); m_real = nullptr; }
        delete this;
    }
    return (ULONG)r;
}

// ---------------------------------------------------------------------------
// IDirect3D9 — passthrough except CreateDevice (wraps the returned device)
// ---------------------------------------------------------------------------
HRESULT D3D9Proxy::RegisterSoftwareDevice(void* p)                                                  { return m_real->RegisterSoftwareDevice(p); }
UINT    D3D9Proxy::GetAdapterCount()                                                                { return m_real->GetAdapterCount(); }
HRESULT D3D9Proxy::GetAdapterIdentifier(UINT A, DWORD F, D3DADAPTER_IDENTIFIER9* p)                 { return m_real->GetAdapterIdentifier(A, F, p); }
UINT    D3D9Proxy::GetAdapterModeCount(UINT A, D3DFORMAT F)                                         { return m_real->GetAdapterModeCount(A, F); }
HRESULT D3D9Proxy::EnumAdapterModes(UINT A, D3DFORMAT F, UINT M, D3DDISPLAYMODE* p)                 { return m_real->EnumAdapterModes(A, F, M, p); }
HRESULT D3D9Proxy::GetAdapterDisplayMode(UINT A, D3DDISPLAYMODE* p)                                 { return m_real->GetAdapterDisplayMode(A, p); }
HRESULT D3D9Proxy::CheckDeviceType(UINT A, D3DDEVTYPE D, D3DFORMAT AF, D3DFORMAT BF, BOOL W)        { return m_real->CheckDeviceType(A, D, AF, BF, W); }
HRESULT D3D9Proxy::CheckDeviceFormat(UINT A, D3DDEVTYPE D, D3DFORMAT AF, DWORD U, D3DRESOURCETYPE R, D3DFORMAT CF) { return m_real->CheckDeviceFormat(A, D, AF, U, R, CF); }
HRESULT D3D9Proxy::CheckDeviceMultiSampleType(UINT A, D3DDEVTYPE D, D3DFORMAT SF, BOOL W, D3DMULTISAMPLE_TYPE MT, DWORD* pQ) { return m_real->CheckDeviceMultiSampleType(A, D, SF, W, MT, pQ); }
HRESULT D3D9Proxy::CheckDepthStencilMatch(UINT A, D3DDEVTYPE D, D3DFORMAT AF, D3DFORMAT RTF, D3DFORMAT DSF) { return m_real->CheckDepthStencilMatch(A, D, AF, RTF, DSF); }
HRESULT D3D9Proxy::CheckDeviceFormatConversion(UINT A, D3DDEVTYPE D, D3DFORMAT SF, D3DFORMAT TF)    { return m_real->CheckDeviceFormatConversion(A, D, SF, TF); }
HRESULT D3D9Proxy::GetDeviceCaps(UINT A, D3DDEVTYPE D, D3DCAPS9* p)                                 { return m_real->GetDeviceCaps(A, D, p); }
HMONITOR D3D9Proxy::GetAdapterMonitor(UINT A)                                                       { return m_real->GetAdapterMonitor(A); }

HRESULT D3D9Proxy::CreateDevice(
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    IDirect3DDevice9** ppReturnedDeviceInterface)
{
    if (!ppReturnedDeviceInterface) return D3DERR_INVALIDCALL;

    D3DPRESENT_PARAMETERS modified;
    D3DPRESENT_PARAMETERS* pParams = pPresentationParameters;
    UINT logicalW = 0, logicalH = 0;
    if (pParams)
    {
        modified = *pParams;
        ResolveAndDoubleSwapchainParams(&modified, hFocusWindow, &logicalW, &logicalH);
        pParams = &modified;
    }

    IDirect3DDevice9* realDevice = nullptr;
    HRESULT hr = m_real->CreateDevice(Adapter, DeviceType, hFocusWindow,
                                      BehaviorFlags, pParams,
                                      &realDevice);
    if (FAILED(hr) || !realDevice)
    {
        *ppReturnedDeviceInterface = nullptr;
        return hr;
    }
    auto* proxy = new Device9Proxy(realDevice, /*isEx=*/false);
    if (logicalW > 0) proxy->SetLogicalBackBufferSize(logicalW, logicalH);
    proxy->StashBackBufferReference();
    *ppReturnedDeviceInterface = proxy;
    return hr;
}

// ---------------------------------------------------------------------------
// IDirect3D9Ex extras — only reachable when m_isEx (QI gates the cast)
// ---------------------------------------------------------------------------
UINT D3D9Proxy::GetAdapterModeCountEx(UINT A, CONST D3DDISPLAYMODEFILTER* pF)                              { return m_realEx->GetAdapterModeCountEx(A, pF); }
HRESULT D3D9Proxy::EnumAdapterModesEx(UINT A, CONST D3DDISPLAYMODEFILTER* pF, UINT M, D3DDISPLAYMODEEX* p) { return m_realEx->EnumAdapterModesEx(A, pF, M, p); }
HRESULT D3D9Proxy::GetAdapterDisplayModeEx(UINT A, D3DDISPLAYMODEEX* p, D3DDISPLAYROTATION* pR)            { return m_realEx->GetAdapterDisplayModeEx(A, p, pR); }
HRESULT D3D9Proxy::GetAdapterLUID(UINT A, LUID* p)                                                         { return m_realEx->GetAdapterLUID(A, p); }

HRESULT D3D9Proxy::CreateDeviceEx(
    UINT Adapter,
    D3DDEVTYPE DeviceType,
    HWND hFocusWindow,
    DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS* pPresentationParameters,
    D3DDISPLAYMODEEX* pFullscreenDisplayMode,
    IDirect3DDevice9Ex** ppReturnedDeviceInterface)
{
    if (!ppReturnedDeviceInterface) return D3DERR_INVALIDCALL;

    D3DPRESENT_PARAMETERS modified;
    D3DPRESENT_PARAMETERS* pParams = pPresentationParameters;
    UINT logicalW = 0, logicalH = 0;
    if (pParams)
    {
        modified = *pParams;
        ResolveAndDoubleSwapchainParams(&modified, hFocusWindow, &logicalW, &logicalH);
        pParams = &modified;
    }

    IDirect3DDevice9Ex* realDevice = nullptr;
    HRESULT hr = m_realEx->CreateDeviceEx(Adapter, DeviceType, hFocusWindow,
                                          BehaviorFlags, pParams,
                                          pFullscreenDisplayMode, &realDevice);
    if (FAILED(hr) || !realDevice)
    {
        *ppReturnedDeviceInterface = nullptr;
        return hr;
    }
    auto* proxy = new Device9Proxy(realDevice, /*isEx=*/true);
    if (logicalW > 0) proxy->SetLogicalBackBufferSize(logicalW, logicalH);
    proxy->StashBackBufferReference();
    *ppReturnedDeviceInterface = static_cast<IDirect3DDevice9Ex*>(proxy);
    return hr;
}

} // namespace NvDirectMode
