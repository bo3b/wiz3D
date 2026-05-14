/* wiz3D - ID3D10Buffer proxy implementation (Option B for DX10) */

#include "StdAfx.h"
#include "Buffer10Proxy.h"
#include "Device10Proxy.h"
#include "proxy_factory.h"

#pragma comment(lib, "dxguid.lib")

namespace wiz3d
{

Buffer10Proxy::Buffer10Proxy(ID3D10Buffer* real, Device10Proxy* parent)
    : m_real(real)
    , m_parent(parent)
    , m_refs(1)
    , m_vsBound(false)
{
}

Buffer10Proxy::~Buffer10Proxy()
{
    if (m_real) { m_real->Release(); m_real = nullptr; }
}

ULONG STDMETHODCALLTYPE Buffer10Proxy::Release()
{
    LONG r = InterlockedDecrement(&m_refs);
    if (r == 0) delete this;
    return (ULONG)r;
}

HRESULT STDMETHODCALLTYPE Buffer10Proxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown        ||
        riid == IID_ID3D10DeviceChild ||
        riid == IID_ID3D10Resource  ||
        riid == IID_ID3D10Buffer)
    {
        *ppvObj = static_cast<ID3D10Buffer*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_wiz3D_Buffer10Proxy)
    {
        *ppvObj = static_cast<IUnknown*>(static_cast<ID3D10Buffer*>(this));
        AddRef();
        return S_OK;
    }
    return m_real->QueryInterface(riid, ppvObj);
}

void STDMETHODCALLTYPE Buffer10Proxy::GetDevice(ID3D10Device** ppDevice)
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
