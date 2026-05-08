#include "proxy_factory.h"
#include "Device11Proxy.h"
#include "Context11Proxy.h"
#include "SwapChainProxy.h"

namespace NvDirectMode
{

void WrapD3D11DeviceAndContext(void** ppDeviceInOut, void** ppContextInOut)
{
    if (!ppDeviceInOut || !*ppDeviceInOut) return;

    auto* realDevice  = static_cast<ID3D11Device*>(*ppDeviceInOut);
    auto* deviceProxy = new Device11Proxy(realDevice);

    if (ppContextInOut && *ppContextInOut)
    {
        auto* realCtx  = static_cast<ID3D11DeviceContext*>(*ppContextInOut);
        auto* ctxProxy = new Context11Proxy(realCtx, deviceProxy);
        deviceProxy->SetImmediateContextProxy(ctxProxy);
        *ppContextInOut = static_cast<ID3D11DeviceContext*>(ctxProxy);
    }

    *ppDeviceInOut = static_cast<ID3D11Device*>(deviceProxy);
}

void* WrapDXGISwapChain(void* realSwapChain, void* wrappedDevice)
{
    if (!realSwapChain) return nullptr;
    auto* sc       = static_cast<IDXGISwapChain*>(realSwapChain);
    auto* devProxy = static_cast<Device11Proxy*>(wrappedDevice);
    auto* scProxy  = new SwapChainProxy(sc, devProxy);
    return static_cast<IDXGISwapChain*>(scProxy);
}

void SetWrappedDeviceLogicalSize(void* wrappedDevice, unsigned int w, unsigned int h)
{
    if (!wrappedDevice) return;
    static_cast<Device11Proxy*>(wrappedDevice)->SetLogicalBackBufferSize(w, h);
}

} // namespace NvDirectMode
