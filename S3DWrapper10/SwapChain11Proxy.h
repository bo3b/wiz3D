/* wiz3D - IDXGISwapChain / IDXGISwapChain1 proxy (Option B Stage 4b.2)
 *
 * Lightweight COM-vtable wrap of IDXGISwapChain (+ IDXGISwapChain1 via QI).
 * Most methods passthrough. The Present and Present1 entries fire a frame-
 * end callback on the parent Device11Proxy's immediate Context11Proxy so the
 * recording machinery has a flush + replay trigger.
 *
 * Stage 4b.2 ships the class + IID; the factory hook that actually produces
 * the proxies at CreateSwapChain time lands in 4b.3.
 *
 * Minimal vs the existing DXGISwapChainWrapper (ATL-based, for the legacy
 * DDI path) and NvDirectMode/d3d11/SwapChainProxy (1300+ lines with shadow-
 * RT + magic-header capture). The Option B path doesn't need either — we
 * keep the real BB at native size and SBS-composite into it at Present time
 * (Stage 4d).
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9types.h>   // D3DCOLORVALUE for the DXGI_RGBA shim below
#ifndef _DXGI_RGBA_DEFINED
#define _DXGI_RGBA_DEFINED
typedef D3DCOLORVALUE DXGI_RGBA;     // see DXGIDeviceProxy.h — bundled
                                     // lib/d3d10 dxgitype.h shadows the
                                     // SDK header and is missing this.
#endif
#include <dxgi.h>
#include <dxgi1_2.h>

namespace wiz3d
{

class Device11Proxy;

class SwapChain11Proxy : public IDXGISwapChain1
{
public:
    // real    : the underlying IDXGISwapChain from the real DXGI factory.
    // parent  : the Device11Proxy this swap chain belongs to (not AddRef'd
    //           by us — parent always outlives the swap chain because
    //           Release-cascade from game holds it alive).
    // real1   : optional QI'd IDXGISwapChain1 (Win8+). Nullable; methods on
    //           the IDXGISwapChain1 surface return E_NOINTERFACE when this
    //           is null.
    SwapChain11Proxy(IDXGISwapChain* real, IDXGISwapChain1* real1, Device11Proxy* parent);
    virtual ~SwapChain11Proxy();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG   STDMETHODCALLTYPE AddRef() override                                  { return InterlockedIncrement(&m_refs); }
    ULONG   STDMETHODCALLTYPE Release() override;

    // IDXGIObject
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData) override          { return m_real->SetPrivateData(Name, DataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnk) override             { return m_real->SetPrivateDataInterface(Name, pUnk); }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData) override              { return m_real->GetPrivateData(Name, pDataSize, pData); }
    HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent) override                                 { return m_real->GetParent(riid, ppParent); }

    // IDXGIDeviceSubObject
    HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void** ppDevice) override;

    // IDXGISwapChain
    HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) override;
    HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void** ppSurface) override                   { return m_real->GetBuffer(Buffer, riid, ppSurface); }
    HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget) override               { return m_real->SetFullscreenState(Fullscreen, pTarget); }
    HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget) override           { return m_real->GetFullscreenState(pFullscreen, ppTarget); }
    HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc) override                                    { return m_real->GetDesc(pDesc); }
    HRESULT STDMETHODCALLTYPE ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags) override { return m_real->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags); }
    HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters) override                { return m_real->ResizeTarget(pNewTargetParameters); }
    HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput** ppOutput) override                             { return m_real->GetContainingOutput(ppOutput); }
    HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats) override                       { return m_real->GetFrameStatistics(pStats); }
    HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT* pLastPresentCount) override                            { return m_real->GetLastPresentCount(pLastPresentCount); }

    // IDXGISwapChain1
    HRESULT STDMETHODCALLTYPE GetDesc1(DXGI_SWAP_CHAIN_DESC1* pDesc) override                                  { return m_real1 ? m_real1->GetDesc1(pDesc) : E_NOINTERFACE; }
    HRESULT STDMETHODCALLTYPE GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc) override               { return m_real1 ? m_real1->GetFullscreenDesc(pDesc) : E_NOINTERFACE; }
    HRESULT STDMETHODCALLTYPE GetHwnd(HWND* pHwnd) override                                                    { return m_real1 ? m_real1->GetHwnd(pHwnd) : E_NOINTERFACE; }
    HRESULT STDMETHODCALLTYPE GetCoreWindow(REFIID refiid, void** ppUnk) override                              { return m_real1 ? m_real1->GetCoreWindow(refiid, ppUnk) : E_NOINTERFACE; }
    HRESULT STDMETHODCALLTYPE Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters) override;
    BOOL    STDMETHODCALLTYPE IsTemporaryMonoSupported() override                                              { return m_real1 ? m_real1->IsTemporaryMonoSupported() : FALSE; }
    HRESULT STDMETHODCALLTYPE GetRestrictToOutput(IDXGIOutput** ppRestrictToOutput) override                   { return m_real1 ? m_real1->GetRestrictToOutput(ppRestrictToOutput) : E_NOINTERFACE; }
    HRESULT STDMETHODCALLTYPE SetBackgroundColor(const DXGI_RGBA* pColor) override                             { return m_real1 ? m_real1->SetBackgroundColor(pColor) : E_NOINTERFACE; }
    HRESULT STDMETHODCALLTYPE GetBackgroundColor(DXGI_RGBA* pColor) override                                   { return m_real1 ? m_real1->GetBackgroundColor(pColor) : E_NOINTERFACE; }
    HRESULT STDMETHODCALLTYPE SetRotation(DXGI_MODE_ROTATION Rotation) override                                { return m_real1 ? m_real1->SetRotation(Rotation) : E_NOINTERFACE; }
    HRESULT STDMETHODCALLTYPE GetRotation(DXGI_MODE_ROTATION* pRotation) override                              { return m_real1 ? m_real1->GetRotation(pRotation) : E_NOINTERFACE; }

    // wiz3D accessors
    IDXGISwapChain* GetReal()    const { return m_real;   }
    Device11Proxy*  GetParent()  const { return m_parent; }

private:
    // Shared body for Present / Present1 — fires the frame-end callback on
    // the parent's immediate Context11Proxy before the real Present runs.
    // Returns true if the callback ran (for stats), false if no context.
    void OnPresentBoundary();

    IDXGISwapChain*  m_real;     // owned (released in dtor)
    IDXGISwapChain1* m_real1;    // optional, owned, nullable
    Device11Proxy*   m_parent;   // not owned
    LONG             m_refs;
};

} // namespace wiz3d
