/* wiz3D - ID3D10Texture2D proxy implementation (Option B for DX10) */

#include "StdAfx.h"
#include "Texture2D10Proxy.h"
#include "Device10Proxy.h"
#include "proxy_factory.h"

#pragma comment(lib, "dxguid.lib")

namespace wiz3d
{

Texture2D10Proxy::Texture2D10Proxy(ID3D10Texture2D* realLeft, ID3D10Texture2D* realRight, Device10Proxy* parent)
    : m_realLeft(realLeft)
    , m_realRight(realRight)
    , m_parent(parent)
    , m_refs(1)
{
}

Texture2D10Proxy::~Texture2D10Proxy()
{
    if (m_realRight) { m_realRight->Release(); m_realRight = nullptr; }
    if (m_realLeft)  { m_realLeft->Release();  m_realLeft  = nullptr; }
}

ULONG STDMETHODCALLTYPE Texture2D10Proxy::Release()
{
    LONG r = InterlockedDecrement(&m_refs);
    if (r == 0) delete this;
    return (ULONG)r;
}

HRESULT STDMETHODCALLTYPE Texture2D10Proxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown        ||
        riid == IID_ID3D10DeviceChild ||
        riid == IID_ID3D10Resource  ||
        riid == IID_ID3D10Texture2D)
    {
        *ppvObj = static_cast<ID3D10Texture2D*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_wiz3D_Texture2D10Proxy)
    {
        *ppvObj = static_cast<IUnknown*>(static_cast<ID3D10Texture2D*>(this));
        AddRef();
        return S_OK;
    }
    return m_realLeft->QueryInterface(riid, ppvObj);
}

void STDMETHODCALLTYPE Texture2D10Proxy::GetDevice(ID3D10Device** ppDevice)
{
    if (!ppDevice) return;
    if (m_parent)
    {
        *ppDevice = static_cast<ID3D10Device*>(m_parent);
        m_parent->AddRef();
        return;
    }
    m_realLeft->GetDevice(ppDevice);
}

} // namespace wiz3d
