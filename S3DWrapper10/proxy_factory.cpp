/* wiz3D d3d11 - COM-proxy factory bridge implementation (Option B Stage 2).
 *
 * Ported pattern from NvDirectMode/d3d11/proxy_factory.cpp. The export
 * wiz3D_WrapD3D11DeviceAndContext is the integration point the d3d11.dll
 * proxy (wiz3D-proxy/d3d11/dllmain.cpp) calls right after the real
 * D3D11CreateDevice returns, swapping the returned device + immediate
 * context for our wrappers before the game sees them.
 */

#include "StdAfx.h"
#include "proxy_factory.h"
#include "Device11Proxy.h"
#include "Context11Proxy.h"
#include "AdapterFunctions.h"  // DDILog

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// {3D8F1B2E-7A4C-4D6E-B1F0-8C3A9D2E5F6B}
// Private GUID claimed by Device11Proxy::QueryInterface so cross-DLL identity
// checks (the dxgi.dll proxy probing whether a CreateSwapChain device is one
// of ours) work via memcmp on the IID alone — no shared symbols needed.
EXTERN_C const GUID IID_wiz3D_Device11Proxy =
    { 0x3D8F1B2E, 0x7A4C, 0x4D6E, { 0xB1, 0xF0, 0x8C, 0x3A, 0x9D, 0x2E, 0x5F, 0x6B } };

namespace wiz3d
{

void WrapD3D11DeviceAndContext(void** ppDeviceInOut, void** ppContextInOut)
{
    if (!ppDeviceInOut || !*ppDeviceInOut) return;

    auto* realDevice  = static_cast<ID3D11Device*>(*ppDeviceInOut);
    auto* deviceProxy = new Device11Proxy(realDevice);
    DDILog("WrapD3D11DeviceAndContext: realDevice=%p -> wiz3d::Device11Proxy=%p\n",
           realDevice, deviceProxy);

    if (ppContextInOut && *ppContextInOut)
    {
        auto* realCtx  = static_cast<ID3D11DeviceContext*>(*ppContextInOut);
        auto* ctxProxy = new Context11Proxy(realCtx, deviceProxy);
        deviceProxy->SetImmediateContextProxy(ctxProxy);
        DDILog("WrapD3D11DeviceAndContext: realCtx=%p -> wiz3d::Context11Proxy=%p\n",
               realCtx, ctxProxy);
        *ppContextInOut = static_cast<ID3D11DeviceContext*>(ctxProxy);
    }

    *ppDeviceInOut = static_cast<ID3D11Device*>(deviceProxy);
}

} // namespace wiz3d

extern "C" __declspec(dllexport) void
wiz3D_WrapD3D11DeviceAndContext(void** ppDeviceInOut, void** ppContextInOut)
{
    wiz3d::WrapD3D11DeviceAndContext(ppDeviceInOut, ppContextInOut);
}
