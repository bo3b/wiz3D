#include "swapchain_helpers.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>

// Output-mode flag from dllmain (avoids dragging log.h's d3d11 deps in here).
extern "C" int NvDM_OutputIsTopBottom();

// NOTE: this TU intentionally includes ONLY <windows.h> + <dxgi.h>.
// Do not pull in d3d11.h or any of the NvDirectMode proxy headers here —
// that's what tripped the MSVC parser cascade described in swapchain_helpers.h.

namespace NvDirectMode
{

const void* MakeDoubledSwapChainDesc(const void* pSwapChainDesc,
                                     unsigned int* outLogicalW,
                                     unsigned int* outLogicalH)
{
    // Stage 3 shadow-RT: this helper used to double the desc so the real
    // swap chain held a 2W x H side-by-side BB. With shadow-RT the real
    // BB stays at the game's requested (one-eye) size and we keep the
    // doubled rendering surface in a side-allocated ID3D11Texture2D
    // (managed by SwapChainProxy::EnsureShadowBB). So this function now
    // only extracts the size from the desc; the desc itself is returned
    // unchanged.
    if (outLogicalW) *outLogicalW = 0;
    if (outLogicalH) *outLogicalH = 0;
    if (!pSwapChainDesc) return nullptr;

    auto* d = (const DXGI_SWAP_CHAIN_DESC*)pSwapChainDesc;
    UINT w = d->BufferDesc.Width;
    UINT h = d->BufferDesc.Height;
    if ((w == 0 || h == 0) && d->OutputWindow)
    {
        RECT rc = { 0 };
        if (GetClientRect(d->OutputWindow, &rc))
        {
            if (w == 0) w = (UINT)(rc.right - rc.left);
            if (h == 0) h = (UINT)(rc.bottom - rc.top);
        }
    }
    if (outLogicalW) *outLogicalW = (unsigned int)w;
    if (outLogicalH) *outLogicalH = (unsigned int)h;
    return pSwapChainDesc;
}

const void* MakeDoubledSwapChainDesc1(const void* pSwapChainDesc1,
                                      unsigned int* outLogicalW,
                                      unsigned int* outLogicalH)
{
    // See note in MakeDoubledSwapChainDesc — shadow-RT architecture
    // means we no longer mutate the desc; just extract the size.
    if (outLogicalW) *outLogicalW = 0;
    if (outLogicalH) *outLogicalH = 0;
    if (!pSwapChainDesc1) return nullptr;

    auto* d = (const DXGI_SWAP_CHAIN_DESC1*)pSwapChainDesc1;
    if (outLogicalW) *outLogicalW = (unsigned int)d->Width;
    if (outLogicalH) *outLogicalH = (unsigned int)d->Height;
    return pSwapChainDesc1;
}

} // namespace NvDirectMode
