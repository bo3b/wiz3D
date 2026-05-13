/* wiz3D d3d11 - COM-proxy factory bridge between the d3d11.dll proxy and the
 * Device11Proxy / Context11Proxy classes. Inputs/outputs typed as void* so the
 * d3d11.dll proxy TU stays free of d3d11.h (system header re-declares
 * D3D11CreateDevice without __declspec(dllexport), conflicting with our
 * exports). Mirrors NvDirectMode/d3d11/proxy_factory.h's role for the iZ3D
 * stereo wrapper Option B port.
 */

#pragma once

#include <guiddef.h>

// Private IID claimed by Device11Proxy::QueryInterface so other DLLs can
// detect "is this a wiz3D-wrapped device?" via the standard COM equality path
// without sharing any C++ symbols. The wiz3D dxgi.dll proxy declares its own
// const GUID with the same value and matches on memcmp.
EXTERN_C const GUID IID_wiz3D_Device11Proxy;

namespace wiz3d
{
    // Wraps the device + (optional) immediate context the system
    // D3D11CreateDevice path produced. Both inputs and outputs are passed
    // as void* so the d3d11.dll dllmain TU stays away from d3d11.h.
    //
    // *ppDeviceInOut    : in/out — system device on entry, Device11Proxy on exit
    // *ppContextInOut   : in/out — system immediate context on entry,
    //                     Context11Proxy on exit. May be NULL if the game
    //                     didn't ask for the context.
    void WrapD3D11DeviceAndContext(void** ppDeviceInOut, void** ppContextInOut);
}

// Exported entry point the d3d11.dll proxy resolves via GetProcAddress, so
// the existing wiz3D-proxy/d3d11/dllmain doesn't need to be linked against
// S3DWrapperD3D10.lib directly. Uses __cdecl (the default) rather than WINAPI
// so the exported symbol name matches across x86/x64 — __stdcall would
// decorate to _name@N on x86 and break the bare-name def-file entry.
extern "C" __declspec(dllexport) void
wiz3D_WrapD3D11DeviceAndContext(void** ppDeviceInOut, void** ppContextInOut);
