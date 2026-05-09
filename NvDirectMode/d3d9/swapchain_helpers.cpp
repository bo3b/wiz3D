#include "swapchain_helpers.h"

extern "C" int NvDM_OutputIsTopBottom();

namespace NvDirectMode
{

void ResolveAndDoubleSwapchainParams(D3DPRESENT_PARAMETERS* p,
                                     HWND fallbackWindow,
                                     UINT* outLogicalW,
                                     UINT* outLogicalH)
{
    if (!p) return;

    if (p->BackBufferWidth == 0 || p->BackBufferHeight == 0)
    {
        HWND hwnd = p->hDeviceWindow ? p->hDeviceWindow : fallbackWindow;
        RECT rc = { 0 };
        if (hwnd && GetClientRect(hwnd, &rc))
        {
            if (p->BackBufferWidth  == 0) p->BackBufferWidth  = (UINT)(rc.right  - rc.left);
            if (p->BackBufferHeight == 0) p->BackBufferHeight = (UINT)(rc.bottom - rc.top);
        }
    }

    if (outLogicalW) *outLogicalW = p->BackBufferWidth;
    if (outLogicalH) *outLogicalH = p->BackBufferHeight;

    // Stage 3 v2 / shadow-RT (mirror of d3d11/d3d10): no longer doubles
    // the desc. Real BB stays at the game's requested (one-eye) size;
    // the per-eye rendering surface lives in a side IDirect3DSurface9
    // managed by Device9Proxy::EnsureShadow. This helper just resolves
    // any 0x0 dimensions against the window now.
    (void)NvDM_OutputIsTopBottom;
}

} // namespace NvDirectMode
