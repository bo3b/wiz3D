/* wiz3D - ID3D11Texture3D proxy (Option B Stage 3c.2)
 *
 * Passthrough wrap + private IID. No stereo siblings — Tex3D used for
 * volume textures/LUTs, not eye-doubled render targets.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>

namespace wiz3d
{

class Device11Proxy;

class Texture3D11Proxy : public ID3D11Texture3D
{
public:
    Texture3D11Proxy(ID3D11Texture3D* real, Device11Proxy* parent);
    virtual ~Texture3D11Proxy();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG   STDMETHODCALLTYPE AddRef() override                  { return InterlockedIncrement(&m_refs); }
    ULONG   STDMETHODCALLTYPE Release() override;

    void    STDMETHODCALLTYPE GetDevice(ID3D11Device** ppDevice) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData) override                  { return m_real->GetPrivateData(guid, pDataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void* pData) override              { return m_real->SetPrivateData(guid, DataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown* pData) override                { return m_real->SetPrivateDataInterface(guid, pData); }

    void    STDMETHODCALLTYPE GetType(D3D11_RESOURCE_DIMENSION* pResourceDimension) override                       { m_real->GetType(pResourceDimension); }
    void    STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) override                                  { m_real->SetEvictionPriority(EvictionPriority); }
    UINT    STDMETHODCALLTYPE GetEvictionPriority() override                                                       { return m_real->GetEvictionPriority(); }
    void    STDMETHODCALLTYPE GetDesc(D3D11_TEXTURE3D_DESC* pDesc) override                                        { m_real->GetDesc(pDesc); }

    ID3D11Texture3D*  GetReal()   const { return m_real;   }
    Device11Proxy*    GetParent() const { return m_parent; }

private:
    ID3D11Texture3D* m_real;
    Device11Proxy*   m_parent;
    LONG             m_refs;
};

} // namespace wiz3d
