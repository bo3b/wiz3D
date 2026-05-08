/* NvDirectMode - thin factory bridge between dllmain.cpp and the COM proxies.
 *
 * dllmain.cpp must NOT include d3d9.h: doing so pulls in system declarations
 * of D3DPERF_* / Direct3DCreate9* with no `__declspec(dllexport)`, which
 * conflicts in linkage with our own `extern "C" __declspec(dllexport)`
 * definitions of those same symbols. Routing proxy creation through this
 * void*-typed factory keeps d3d9.h confined to the proxy translation units.
 */

#pragma once

namespace NvDirectMode
{
    // Wrap a raw IDirect3D9* in a D3D9Proxy. Returns the wrapped pointer
    // (still typed IDirect3D9* under the hood) as void* so callers don't
    // need d3d9.h.
    void* CreateD3D9Proxy(void* realD3D9);

    // Same, but for the Ex flavour — the void* is actually IDirect3D9Ex*
    // and the proxy's QI for IID_IDirect3D9Ex will succeed.
    void* CreateD3D9ExProxy(void* realD3D9Ex);
}
