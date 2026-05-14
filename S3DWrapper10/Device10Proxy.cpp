/* wiz3D - ID3D10Device proxy implementation (Option B for DX10) */

#include "StdAfx.h"
#include "Device10Proxy.h"
#include "proxy_factory.h"     // IID_wiz3D_Device10Proxy

#pragma comment(lib, "dxguid.lib")

namespace wiz3d
{

Device10Proxy::Device10Proxy(ID3D10Device* real)
    : m_real(real)
    , m_refs(1)
{
}

Device10Proxy::~Device10Proxy()
{
}

HRESULT STDMETHODCALLTYPE Device10Proxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_ID3D10Device)
    {
        *ppvObj = static_cast<ID3D10Device*>(this);
        AddRef();
        return S_OK;
    }
    // Private IID for cross-DLL identity (DXGIFactoryWrapper's Option B
    // factory hook QIs incoming pDevice to detect a Device10Proxy).
    if (riid == IID_wiz3D_Device10Proxy)
    {
        *ppvObj = static_cast<IUnknown*>(static_cast<ID3D10Device*>(this));
        AddRef();
        return S_OK;
    }
    // ID3D10Device1 / IDXGIDevice etc — pass through unwrapped for now.
    // Stage 3 of the DX10 port can extend this for COM identity preservation.
    return m_real->QueryInterface(riid, ppvObj);
}

} // namespace wiz3d
