#include "swapchain_helpers.h"

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

    if (p->BackBufferWidth > 0)
        p->BackBufferWidth *= 2;
}

} // namespace NvDirectMode
