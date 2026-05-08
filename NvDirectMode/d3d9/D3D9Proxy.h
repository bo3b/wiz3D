/* NvDirectMode - IDirect3D9 proxy
 *
 * Wraps the IDirect3D9 returned by Direct3DCreate9 so we can intercept
 * CreateDevice and substitute our own Device9Proxy. Stage 1b-i: pure
 * passthrough on every other method. Stage 1b-iii will modify the
 * D3DPRESENT_PARAMETERS' BackBufferWidth on the way through CreateDevice
 * to allocate a doubled back-buffer.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

namespace NvDirectMode
{

class D3D9Proxy : public IDirect3D9Ex
{
public:
    // isEx=true means `real` is actually an IDirect3D9Ex; QueryInterface
    // for IID_IDirect3D9Ex returns this proxy. isEx=false hides the Ex face
    // behind QI E_NOINTERFACE so the proxy looks identical to a plain
    // IDirect3D9 and the 5 Ex methods are unreachable.
    D3D9Proxy(IDirect3D9* real, bool isEx);
    virtual ~D3D9Proxy();

    // IUnknown
    HRESULT  STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG    STDMETHODCALLTYPE AddRef() override;
    ULONG    STDMETHODCALLTYPE Release() override;

    // IDirect3D9
    HRESULT STDMETHODCALLTYPE RegisterSoftwareDevice(void* pInitializeFunction) override;
    UINT    STDMETHODCALLTYPE GetAdapterCount() override;
    HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) override;
    UINT    STDMETHODCALLTYPE GetAdapterModeCount(UINT Adapter, D3DFORMAT Format) override;
    HRESULT STDMETHODCALLTYPE EnumAdapterModes(UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceType(UINT Adapter, D3DDEVTYPE DevType, D3DFORMAT AdapterFormat, D3DFORMAT BackBufferFormat, BOOL bWindowed) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, BOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels) override;
    HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) override;
    HRESULT STDMETHODCALLTYPE CheckDeviceFormatConversion(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) override;
    HRESULT STDMETHODCALLTYPE GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) override;
    HMONITOR STDMETHODCALLTYPE GetAdapterMonitor(UINT Adapter) override;
    HRESULT STDMETHODCALLTYPE CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DDevice9** ppReturnedDeviceInterface) override;

    // IDirect3D9Ex (only meaningful when m_isEx)
    UINT     STDMETHODCALLTYPE GetAdapterModeCountEx(UINT Adapter, CONST D3DDISPLAYMODEFILTER* pFilter) override;
    HRESULT  STDMETHODCALLTYPE EnumAdapterModesEx(UINT Adapter, CONST D3DDISPLAYMODEFILTER* pFilter, UINT Mode, D3DDISPLAYMODEEX* pMode) override;
    HRESULT  STDMETHODCALLTYPE GetAdapterDisplayModeEx(UINT Adapter, D3DDISPLAYMODEEX* pMode, D3DDISPLAYROTATION* pRotation) override;
    HRESULT  STDMETHODCALLTYPE CreateDeviceEx(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, D3DDISPLAYMODEEX* pFullscreenDisplayMode, IDirect3DDevice9Ex** ppReturnedDeviceInterface) override;
    HRESULT  STDMETHODCALLTYPE GetAdapterLUID(UINT Adapter, LUID* pLUID) override;

private:
    IDirect3D9*    m_real;
    IDirect3D9Ex*  m_realEx;   // == m_real reinterpreted, only valid when m_isEx
    bool           m_isEx;
    LONG           m_refs;
};

} // namespace NvDirectMode
