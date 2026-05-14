/* wiz3D - ID3D10DepthStencilView proxy (Option B for DX10, Stage 3 port) */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d10.h>

namespace wiz3d
{

class Device10Proxy;

class DSV10Proxy : public ID3D10DepthStencilView
{
public:
    DSV10Proxy(ID3D10DepthStencilView* realLeft, ID3D10DepthStencilView* realRight, Device10Proxy* parent);
    virtual ~DSV10Proxy();

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObj) override;
    ULONG   STDMETHODCALLTYPE AddRef() override                  { return InterlockedIncrement(&m_refs); }
    ULONG   STDMETHODCALLTYPE Release() override;

    void    STDMETHODCALLTYPE GetDevice(ID3D10Device** ppDevice) override;
    HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID guid, UINT* pDataSize, void* pData) override                  { return m_realLeft->GetPrivateData(guid, pDataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, UINT DataSize, const void* pData) override              { return m_realLeft->SetPrivateData(guid, DataSize, pData); }
    HRESULT STDMETHODCALLTYPE SetPrivateDataInterface(REFGUID guid, const IUnknown* pData) override                { return m_realLeft->SetPrivateDataInterface(guid, pData); }

    void    STDMETHODCALLTYPE GetResource(ID3D10Resource** ppResource) override;
    void    STDMETHODCALLTYPE GetDesc(D3D10_DEPTH_STENCIL_VIEW_DESC* pDesc) override                               { m_realLeft->GetDesc(pDesc); }

    bool                    IsStereo()      const { return m_realRight != nullptr; }
    ID3D10DepthStencilView* GetReal()       const { return m_realLeft;  }
    ID3D10DepthStencilView* GetRealRight()  const { return m_realRight; }
    Device10Proxy*          GetParent()     const { return m_parent;    }

private:
    ID3D10DepthStencilView* m_realLeft;
    ID3D10DepthStencilView* m_realRight;  // nullable
    Device10Proxy*          m_parent;
    LONG                    m_refs;
};

} // namespace wiz3d
