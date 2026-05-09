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

    // OutputMode picks which axis we double:
    //   T-B → backbuffer is W x 2H (eyes stacked)
    //   SBS → backbuffer is 2W x H (eyes side-by-side, default)
    if (NvDM_OutputIsTopBottom())
    {
        if (p->BackBufferHeight > 0) p->BackBufferHeight *= 2;
    }
    else
    {
        if (p->BackBufferWidth  > 0) p->BackBufferWidth  *= 2;
    }
}

} // namespace NvDirectMode
