/* wiz3D - ID3D11Texture3D proxy implementation */

#include "StdAfx.h"
#include "Texture3D11Proxy.h"
#include "Device11Proxy.h"
#include "proxy_factory.h"

#pragma comment(lib, "dxguid.lib")

namespace wiz3d
{

Texture3D11Proxy::Texture3D11Proxy(ID3D11Texture3D* real, Device11Proxy* parent)
    : m_real(real)
    , m_parent(parent)
    , m_refs(1)
{
}

Texture3D11Proxy::~Texture3D11Proxy()
{
    if (m_real) { m_real->Release(); m_real = nullptr; }
}

ULONG STDMETHODCALLTYPE Texture3D11Proxy::Release()
{
    LONG r = InterlockedDecrement(&m_refs);
    if (r == 0) delete this;
    return (ULONG)r;
}

HRESULT STDMETHODCALLTYPE Texture3D11Proxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown          ||
        riid == IID_ID3D11DeviceChild ||
        riid == IID_ID3D11Resource    ||
        riid == IID_ID3D11Texture3D)
    {
        *ppvObj = static_cast<ID3D11Texture3D*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_wiz3D_Texture3D11Proxy)
    {
        *ppvObj = static_cast<IUnknown*>(static_cast<ID3D11Texture3D*>(this));
        AddRef();
        return S_OK;
    }
    return m_real->QueryInterface(riid, ppvObj);
}

void STDMETHODCALLTYPE Texture3D11Proxy::GetDevice(ID3D11Device** ppDevice)
{
    if (!ppDevice) return;
    if (m_parent)
    {
        *ppDevice = static_cast<ID3D11Device*>(m_parent);
        m_parent->AddRef();
        return;
    }
    m_real->GetDevice(ppDevice);
}

} // namespace wiz3d
