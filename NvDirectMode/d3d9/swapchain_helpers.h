/* NvDirectMode d3d9 - shared helper for the back-buffer doubling logic
 *
 * Used by D3D9Proxy (CreateDevice / CreateDeviceEx) and Device9Proxy
 * (Reset / ResetEx) to mutate D3DPRESENT_PARAMETERS on its way to the
 * real D3D before forwarding.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>

namespace NvDirectMode
{
    // Resolves any zero-valued BackBufferWidth/Height against the window
    // client area, then doubles the width so the real swap-chain holds a
    // side-by-side surface for both eyes.
    //
    // outLogicalW / outLogicalH receive the game's perceived (one-eye) size.
    // Caller stashes those for SetRenderTarget viewport routing in 1b-iv.
    //
    // Known limitation (1b-iii): only safe for windowed mode. Fullscreen
    // would require BackBufferWidth*2 to be a valid display mode, which it
    // generally is not. Fixing this needs an internal doubled RT separate
    // from the swap-chain — deferred until we hit a game that needs it.
    void ResolveAndDoubleSwapchainParams(D3DPRESENT_PARAMETERS* p,
                                         HWND fallbackWindow,
                                         UINT* outLogicalW,
                                         UINT* outLogicalH);
}
