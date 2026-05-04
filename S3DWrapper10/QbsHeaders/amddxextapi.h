// amddxextapi.h
// Minimal AMD QBS interface declarations for the iZ3D DX10/11 wrapper.
// Mirrors the public AMD GPU SDK headers (AmdDxExtIface.h + AmdDxExtApi.h),
// trimmed to just what S3DWrapper10/D3DDeviceWrapper.cpp uses.
//
// AmdDxExtCreate / AmdDxExtCreate11 are intentionally NOT declared here.
// The wrapper resolves them dynamically via GetProcAddress on atidxx32/64.dll
// (see D3DDeviceWrapper::OpenAMDStereoInterfaceX).

#ifndef AMDDXEXTAPI_H
#define AMDDXEXTAPI_H

#include <dxgi.h>

// ---------------------------------------------------------------------------
// From AmdDxExt.h
// ---------------------------------------------------------------------------
enum AmdDxExtPrimitiveTopology
{
    AmdDxExtPrimitiveTopology_Undefined         = 0,
    AmdDxExtPrimitiveTopology_PointList         = 1,
    AmdDxExtPrimitiveTopology_LineList          = 2,
    AmdDxExtPrimitiveTopology_LineStrip         = 3,
    AmdDxExtPrimitiveTopology_TriangleList      = 4,
    AmdDxExtPrimitiveTopology_TriangleStrip     = 5,
    AmdDxExtPrimitiveTopology_ExtQuadList       = 7,
    AmdDxExtPrimitiveTopology_RectList          = 8,
    AmdDxExtPrimitiveTopology_LineListAdj       = 10,
    AmdDxExtPrimitiveTopology_LineStripAdj      = 11,
    AmdDxExtPrimitiveTopology_TriangleListAdj   = 12,
    AmdDxExtPrimitiveTopology_TriangleStripAdj  = 13,
    AmdDxExtPrimitiveTopology_Max               = 14
};

// ---------------------------------------------------------------------------
// From AmdDxExtIface.h — base interface (AddRef / Release)
// ---------------------------------------------------------------------------
class IAmdDxExtInterface
{
public:
    virtual unsigned int AddRef(void)  = 0;
    virtual unsigned int Release(void) = 0;

protected:
    IAmdDxExtInterface() {}
    virtual ~IAmdDxExtInterface() = 0 {};
};

// ---------------------------------------------------------------------------
// From AmdDxExtApi.h — version struct + main extension interface
// ---------------------------------------------------------------------------
struct AmdDxExtVersion
{
    unsigned int majorVersion;
    unsigned int minorVersion;
};

interface ID3D10Device;
interface ID3D11Device;
interface ID3D10Resource;
interface ID3D11Resource;

class IAmdDxExt : public IAmdDxExtInterface
{
public:
    virtual HRESULT             GetVersion(AmdDxExtVersion* pExtVer) = 0;
    virtual IAmdDxExtInterface* GetExtInterface(unsigned int iface) = 0;

    virtual HRESULT IaSetPrimitiveTopology(unsigned int topology) = 0;
    virtual HRESULT IaGetPrimitiveTopology(AmdDxExtPrimitiveTopology* pExtTopology) = 0;
    virtual HRESULT SetSingleSampleRead(ID3D10Resource* pResource, BOOL singleSample) = 0;
    virtual HRESULT SetSingleSampleRead11(ID3D11Resource* pResource, BOOL singleSample) = 0;

protected:
    IAmdDxExt() {}
    virtual ~IAmdDxExt() = 0 {};
};

#endif // AMDDXEXTAPI_H
