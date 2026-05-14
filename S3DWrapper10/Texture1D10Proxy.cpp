/* wiz3D - ID3D10Texture1D proxy implementation */

#include "StdAfx.h"
#include "Texture1D10Proxy.h"
#include "Device10Proxy.h"
#include "proxy_factory.h"

#pragma comment(lib, "dxguid.lib")

namespace wiz3d
{

Texture1D10Proxy::Texture1D10Proxy(ID3D10Texture1D* real, Device10Proxy* parent)
    : m_real(real)
    , m_parent(parent)
    , m_refs(1)
{
}

Texture1D10Proxy::~Texture1D10Proxy()
{
    if (m_real) { m_real->Release(); m_real = nullptr; }
}

ULONG STDMETHODCALLTYPE Texture1D10Proxy::Release()
{
    LONG r = InterlockedDecrement(&m_refs);
    if (r == 0) delete this;
    return (ULONG)r;
}

HRESULT STDMETHODCALLTYPE Texture1D10Proxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown          ||
        riid == IID_ID3D10DeviceChild ||
        riid == IID_ID3D10Resource    ||
        riid == IID_ID3D10Texture1D)
    {
        *ppvObj = static_cast<ID3D10Texture1D*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_wiz3D_Texture1D10Proxy)
    {
        *ppvObj = static_cast<IUnknown*>(static_cast<ID3D10Texture1D*>(this));
        AddRef();
        return S_OK;
    }
    return m_real->QueryInterface(riid, ppvObj);
}

void STDMETHODCALLTYPE Texture1D10Proxy::GetDevice(ID3D10Device** ppDevice)
{
    if (!ppDevice) return;
    if (m_parent)
    {
        *ppDevice = static_cast<ID3D10Device*>(m_parent);
        m_parent->AddRef();
        return;
    }
    m_real->GetDevice(ppDevice);
}

} // namespace wiz3d
