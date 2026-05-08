/* NvDirectMode d3d10 - DXGI_SWAP_CHAIN_DESC mutation helper (mirror of d3d11 version)
 *
 * Isolated TU — only includes <windows.h> + <dxgi.h>, no proxy headers.
 * Returns void* to a thread-local doubled-width copy of the desc.
 */

#pragma once

namespace NvDirectMode
{
    const void* MakeDoubledSwapChainDesc(const void* pSwapChainDesc,
                                         unsigned int* outLogicalW,
                                         unsigned int* outLogicalH);
}
