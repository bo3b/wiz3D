// AmdDxExtQbStereoApi.h
// Minimal AMD Quad-Buffer Stereo extension interface for the iZ3D DX10/11 wrapper.
// Mirrors the public AMD GPU SDK header.

#ifndef AMDDXEXTQBSTEREOAPI_H
#define AMDDXEXTQBSTEREOAPI_H

#include <dxgi.h>
#include "amddxextapi.h"

const unsigned int AmdDxExtQuadBufferStereoID = 2;

class IAmdDxExtQuadBufferStereo : public IAmdDxExtInterface
{
public:
    virtual HRESULT EnableQuadBufferStereo(BOOL enable) = 0;

    virtual HRESULT GetDisplayModeList(DXGI_FORMAT EnumFormat,
                                       UINT Flags,
                                       UINT* pNumModes,
                                       DXGI_MODE_DESC* pDesc) = 0;

    virtual UINT GetLineOffset(IDXGISwapChain* pSwapChain) = 0;
};

#endif // AMDDXEXTQBSTEREOAPI_H
