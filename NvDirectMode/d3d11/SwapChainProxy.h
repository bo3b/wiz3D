/* NvDirectMode - IDXGISwapChain proxy
 *
 * Stage 1b-i: passthrough wrapper for IDXGISwapChain. Buffer doubling lands
 * in 1b-iii (ResizeBuffers + GetBuffer become intercept points) and the
 * back-buffer-pointer tracking that 1b-iv's OMSetRenderTargets routing
 * needs hangs off this proxy.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi.h>

namespace NvDirectMode
{

class Device11Proxy;

class SwapChainProxy : public IDXGISwapChain
{
public:
    explicit SwapChainProxy(IDXGISwapChain* real, Device11Proxy* parent);
    virtual ~SwapChainProxy();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG   STDMETHODCALLTYPE AddRef() override                                  { return InterlockedIncrement(&m_refs); }
    ULONG   STDMETHODCALLTYPE Release() override
    {
        LONG r = InterlockedDecrement(&m_refs);
        if (r == 0) { if (m_real) { m_real->Release(); m_real = nullptr; } delete this; }
        return (ULONG)r;
    }

    // IDXGIObject
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID Name, UINT DataSize, const void* pData) override        { return m_real->SetPrivateData(Name, DataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown) override       { return m_real->SetPrivateDataInterface(Name, pUnknown); }
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData) override            { return m_real->GetPrivateData(Name, pDataSize, pData); }
    HRESULT STDMETHODCALLTYPE GetParent(REFIID riid, void** ppParent) override                               { return m_real->GetParent(riid, ppParent); }

    // IDXGIDeviceSubObject
    HRESULT STDMETHODCALLTYPE GetDevice(REFIID riid, void** ppDevice) override                               { return m_real->GetDevice(riid, ppDevice); }

    // IDXGISwapChain
    HRESULT STDMETHODCALLTYPE Present(UINT SyncInterval, UINT Flags) override                                { return m_real->Present(SyncInterval, Flags); }
    HRESULT STDMETHODCALLTYPE GetBuffer(UINT Buffer, REFIID riid, void** ppSurface) override;
    HRESULT STDMETHODCALLTYPE SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget) override             { return m_real->SetFullscreenState(Fullscreen, pTarget); }
    HRESULT STDMETHODCALLTYPE GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget) override         { return m_real->GetFullscreenState(pFullscreen, ppTarget); }
    HRESULT STDMETHODCALLTYPE GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc) override                                  { return m_real->GetDesc(pDesc); }
    HRESULT STDMETHODCALLTYPE ResizeBuffers(
        UINT BufferCount, UINT Width, UINT Height,
        DXGI_FORMAT NewFormat, UINT SwapChainFlags) override
    {
        return m_real->ResizeBuffers(BufferCount, Width, Height, NewFormat, SwapChainFlags);
    }
    HRESULT STDMETHODCALLTYPE ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters) override              { return m_real->ResizeTarget(pNewTargetParameters); }
    HRESULT STDMETHODCALLTYPE GetContainingOutput(IDXGIOutput** ppOutput) override                           { return m_real->GetContainingOutput(ppOutput); }
    HRESULT STDMETHODCALLTYPE GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats) override                     { return m_real->GetFrameStatistics(pStats); }
    HRESULT STDMETHODCALLTYPE GetLastPresentCount(UINT* pLastPresentCount) override                          { return m_real->GetLastPresentCount(pLastPresentCount); }

    // Accessors for 1b-iii / 1b-iv (set up here so we don't have to add them later)
    IDXGISwapChain* GetReal() const { return m_real; }
    Device11Proxy*  GetParent() const { return m_parent; }

private:
    IDXGISwapChain* m_real;
    Device11Proxy*  m_parent;   // not AddRef'd; device outlives swap chain
    LONG            m_refs;
};

} // namespace NvDirectMode
