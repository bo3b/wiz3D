#include "proxy_factory.h"
#include "Device10Proxy.h"
#include "SwapChainProxy.h"

namespace NvDirectMode
{

void WrapD3D10Device(void** ppDeviceInOut)
{
    if (!ppDeviceInOut || !*ppDeviceInOut) return;
    auto* realDevice = static_cast<ID3D10Device*>(*ppDeviceInOut);
    auto* devProxy   = new Device10Proxy(realDevice);
    *ppDeviceInOut = static_cast<ID3D10Device*>(devProxy);
}

void* WrapDXGISwapChain(void* realSwapChain, void* wrappedDevice)
{
    if (!realSwapChain) return nullptr;
    auto* sc       = static_cast<IDXGISwapChain*>(realSwapChain);
    auto* devProxy = static_cast<Device10Proxy*>(wrappedDevice);
    auto* scProxy  = new SwapChainProxy(sc, devProxy);
    return static_cast<IDXGISwapChain*>(scProxy);
}

void SetWrappedDeviceLogicalSize(void* wrappedDevice, unsigned int w, unsigned int h)
{
    if (!wrappedDevice) return;
    static_cast<Device10Proxy*>(wrappedDevice)->SetLogicalBackBufferSize(w, h);
}

} // namespace NvDirectMode
