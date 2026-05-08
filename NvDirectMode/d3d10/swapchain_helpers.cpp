#include "swapchain_helpers.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi.h>

namespace NvDirectMode
{

const void* MakeDoubledSwapChainDesc(const void* pSwapChainDesc,
                                     unsigned int* outLogicalW,
                                     unsigned int* outLogicalH)
{
    if (outLogicalW) *outLogicalW = 0;
    if (outLogicalH) *outLogicalH = 0;
    if (!pSwapChainDesc) return nullptr;

    static thread_local DXGI_SWAP_CHAIN_DESC tlsDesc;
    tlsDesc = *(const DXGI_SWAP_CHAIN_DESC*)pSwapChainDesc;

    if (tlsDesc.BufferDesc.Width == 0 || tlsDesc.BufferDesc.Height == 0)
    {
        HWND hwnd = tlsDesc.OutputWindow;
        RECT rc = { 0 };
        if (hwnd && GetClientRect(hwnd, &rc))
        {
            if (tlsDesc.BufferDesc.Width  == 0) tlsDesc.BufferDesc.Width  = (UINT)(rc.right - rc.left);
            if (tlsDesc.BufferDesc.Height == 0) tlsDesc.BufferDesc.Height = (UINT)(rc.bottom - rc.top);
        }
    }

    if (outLogicalW) *outLogicalW = (unsigned int)tlsDesc.BufferDesc.Width;
    if (outLogicalH) *outLogicalH = (unsigned int)tlsDesc.BufferDesc.Height;

    if (tlsDesc.BufferDesc.Width > 0)
        tlsDesc.BufferDesc.Width *= 2;

    return &tlsDesc;
}

} // namespace NvDirectMode
