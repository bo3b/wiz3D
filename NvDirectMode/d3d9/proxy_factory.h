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

    // Layout-stable path (task #61): instead of returning a wrapped
    // D3D9Proxy from Direct3DCreate9 (which fails anti-tamper sniffs in
    // GTA IV / Hard Reset because our vtable isn't where the game expects
    // it), hot-patch the real IDirect3D9 vtable's CreateDevice slot so
    // the game gets the real IDirect3D9 (passing layout sniffs) and our
    // hook fires when the game later calls CreateDevice.
    //
    // realD3D9 is the IDirect3D9 returned by the real Direct3DCreate9.
    // isEx indicates whether to also patch CreateDeviceEx (slot 19) for
    // the Ex flavour. Idempotent: subsequent calls with the same vtable
    // skip already-patched slots. Returns true if patching succeeded.
    bool InstallVtablePatchForD3D9(void* realD3D9, bool isEx);
}
