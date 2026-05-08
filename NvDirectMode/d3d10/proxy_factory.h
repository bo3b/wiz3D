/* NvDirectMode d3d10 - thin factory bridge between dllmain.cpp and the proxies.
 * Same rationale as the d3d11 version — keep d3d10.h / dxgi.h out of dllmain
 * so the export-linkage clash with system declarations doesn't bite.
 */

#pragma once

namespace NvDirectMode
{
    // Wraps the device returned by D3D10CreateDevice. Replaces *ppDeviceInOut
    // in place with the wrapper.
    void WrapD3D10Device(void** ppDeviceInOut);

    // Wraps a swap chain produced via D3D10CreateDeviceAndSwapChain. The
    // device pointer must already be a wrapped Device10Proxy.
    void* WrapDXGISwapChain(void* realSwapChain, void* wrappedDevice);

    // 1b-iii: tag the wrapped device with the game's perceived (one-eye)
    // backbuffer dimensions for OMSetRenderTargets viewport routing.
    void SetWrappedDeviceLogicalSize(void* wrappedDevice, unsigned int w, unsigned int h);
}
