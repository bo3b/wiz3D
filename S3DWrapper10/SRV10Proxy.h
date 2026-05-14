/* wiz3D - ID3D10ShaderResourceView proxy (Option B Stage 3c.2 DX10 port)
 *
 * Mirror of SRV11Proxy for the DX10 path. Carries an optional right-eye
 * sibling SRV so that downstream xxSetShaderResources calls can rebind
 * the right-eye texture during the right-eye replay sweep. Without this,
 * Lost Planet-style games render shifted geometry into the right-eye RT
 * sibling but later passes always sample from the LEFT-eye RT, so the
 * stereo difference never reaches the final composite.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d10.h>

namespace wiz3d
{

class Device10Proxy;

class SRV10Proxy : public ID3D10ShaderResourceView
{
public:
    SRV10Proxy(ID3D10ShaderResourceView* realLeft, ID3D10ShaderResourceView* realRight, Device10Proxy* parent);
    virtual ~SRV10Proxy();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG   STDMETHODCALLTYPE AddRef() override                  { return InterlockedIncrement(&m_refs); }
    ULONG   STDMETHODCALLTYPE Release() override;

    void    STDMETHODCALLTYPE GetDevice(ID3D10Device** ppDevice) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData) override                  { return m_realLeft->GetPrivateData(guid, pDataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void* pData) override              { return m_realLeft->SetPrivateData(guid, DataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown* pData) override                { return m_realLeft->SetPrivateDataInterface(guid, pData); }

    void    STDMETHODCALLTYPE GetResource(ID3D10Resource** ppResource) override                                    { m_realLeft->GetResource(ppResource); }
    void    STDMETHODCALLTYPE GetDesc(D3D10_SHADER_RESOURCE_VIEW_DESC* pDesc) override                             { m_realLeft->GetDesc(pDesc); }

    bool                       IsStereo()      const { return m_realRight != nullptr; }
    ID3D10ShaderResourceView*  GetReal()       const { return m_realLeft;  }
    ID3D10ShaderResourceView*  GetRealRight()  const { return m_realRight; }
    Device10Proxy*             GetParent()     const { return m_parent;    }

private:
    ID3D10ShaderResourceView* m_realLeft;
    ID3D10ShaderResourceView* m_realRight;
    Device10Proxy*            m_parent;
    LONG                      m_refs;
};

} // namespace wiz3d
