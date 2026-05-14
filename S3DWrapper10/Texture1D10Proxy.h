/* wiz3D - ID3D10Texture1D proxy (Option B Stage 3c.2 DX10 port)
 *
 * Passthrough wrap. DX10 Texture1Ds aren't stereo-doubled — they're
 * almost always 1D lookups (gradients, LUTs). The wrap exists so SRV
 * creation paths see a proxy resource and can pass the real underlying
 * pointer to the runtime.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d10.h>

namespace wiz3d
{

class Device10Proxy;

class Texture1D10Proxy : public ID3D10Texture1D
{
public:
    Texture1D10Proxy(ID3D10Texture1D* real, Device10Proxy* parent);
    virtual ~Texture1D10Proxy();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG   STDMETHODCALLTYPE AddRef() override                  { return InterlockedIncrement(&m_refs); }
    ULONG   STDMETHODCALLTYPE Release() override;

    void    STDMETHODCALLTYPE GetDevice(ID3D10Device** ppDevice) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData) override                  { return m_real->GetPrivateData(guid, pDataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void* pData) override              { return m_real->SetPrivateData(guid, DataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown* pData) override                { return m_real->SetPrivateDataInterface(guid, pData); }

    void    STDMETHODCALLTYPE GetType(D3D10_RESOURCE_DIMENSION* pResourceDimension) override                       { m_real->GetType(pResourceDimension); }
    void    STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) override                                  { m_real->SetEvictionPriority(EvictionPriority); }
    UINT    STDMETHODCALLTYPE GetEvictionPriority() override                                                       { return m_real->GetEvictionPriority(); }
    HRESULT STDMETHODCALLTYPE Map(UINT Subresource, D3D10_MAP MapType, UINT MapFlags, void** ppData) override      { return m_real->Map(Subresource, MapType, MapFlags, ppData); }
    void    STDMETHODCALLTYPE Unmap(UINT Subresource) override                                                     { m_real->Unmap(Subresource); }
    void    STDMETHODCALLTYPE GetDesc(D3D10_TEXTURE1D_DESC* pDesc) override                                        { m_real->GetDesc(pDesc); }

    ID3D10Texture1D*  GetReal()   const { return m_real;   }
    Device10Proxy*    GetParent() const { return m_parent; }

private:
    ID3D10Texture1D* m_real;
    Device10Proxy*   m_parent;
    LONG             m_refs;
};

} // namespace wiz3d
