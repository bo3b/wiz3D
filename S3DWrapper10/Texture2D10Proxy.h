/* wiz3D - ID3D10Texture2D proxy (Option B for DX10, Stage 3 port)
 *
 * D3D10 counterpart of Texture2D11Proxy. Wraps the texture returned from
 * Device10Proxy::CreateTexture2D; when the heuristic says doubling is
 * required, the proxy also holds a right-eye sibling.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d10.h>

namespace wiz3d
{

class Device10Proxy;

class Texture2D10Proxy : public ID3D10Texture2D
{
public:
    Texture2D10Proxy(ID3D10Texture2D* realLeft, ID3D10Texture2D* realRight, Device10Proxy* parent);
    virtual ~Texture2D10Proxy();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG   STDMETHODCALLTYPE AddRef() override                  { return InterlockedIncrement(&m_refs); }
    ULONG   STDMETHODCALLTYPE Release() override;

    // ID3D10DeviceChild
    void    STDMETHODCALLTYPE GetDevice(ID3D10Device** ppDevice) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData) override                  { return m_realLeft->GetPrivateData(guid, pDataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void* pData) override              { return m_realLeft->SetPrivateData(guid, DataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown* pData) override                { return m_realLeft->SetPrivateDataInterface(guid, pData); }

    // ID3D10Resource
    void    STDMETHODCALLTYPE GetType(D3D10_RESOURCE_DIMENSION* pResourceDimension) override                       { m_realLeft->GetType(pResourceDimension); }
    void    STDMETHODCALLTYPE SetEvictionPriority(UINT EvictionPriority) override                                  { m_realLeft->SetEvictionPriority(EvictionPriority); if (m_realRight) m_realRight->SetEvictionPriority(EvictionPriority); }
    UINT    STDMETHODCALLTYPE GetEvictionPriority() override                                                       { return m_realLeft->GetEvictionPriority(); }

    // ID3D10Texture2D
    HRESULT STDMETHODCALLTYPE Map(UINT Subresource, D3D10_MAP MapType, UINT MapFlags, D3D10_MAPPED_TEXTURE2D* pMappedTex2D) override { return m_realLeft->Map(Subresource, MapType, MapFlags, pMappedTex2D); }
    void    STDMETHODCALLTYPE Unmap(UINT Subresource) override                                                     { m_realLeft->Unmap(Subresource); }
    void    STDMETHODCALLTYPE GetDesc(D3D10_TEXTURE2D_DESC* pDesc) override                                        { m_realLeft->GetDesc(pDesc); }

    bool             IsStereo()      const { return m_realRight != nullptr; }
    ID3D10Texture2D* GetReal()       const { return m_realLeft;  }
    ID3D10Texture2D* GetRealRight()  const { return m_realRight; }
    Device10Proxy*   GetParent()     const { return m_parent;    }

private:
    ID3D10Texture2D* m_realLeft;
    ID3D10Texture2D* m_realRight;  // nullable
    Device10Proxy*   m_parent;
    LONG             m_refs;
};

} // namespace wiz3d
