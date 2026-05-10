#include "adapter_vtable_hooks.h"
#include "../spoof_identity.h"

#include <dxgi1_2.h>
#include <wchar.h>

#pragma comment(lib, "dxguid.lib")

extern "C" void NvDM_DxgiLog(const char* fmt, ...);

namespace NvDirectMode
{

namespace
{
    // IDXGIAdapter / IDXGIAdapter1 vtable indices. These come from the IDL
    // and are stable across Windows versions.
    constexpr int kSlotAdapterGetDesc   = 8;    // IDXGIAdapter::GetDesc
    constexpr int kSlotAdapter1GetDesc1 = 10;   // IDXGIAdapter1::GetDesc1
                                                // (after IDXGIAdapter's 11
                                                //  slots: 3 IUnknown +
                                                //  4 IDXGIObject +
                                                //  3 IDXGIAdapter +
                                                //  this is the 11th = slot 10)

    typedef HRESULT (STDMETHODCALLTYPE *PFN_GetDesc) (IDXGIAdapter*,  DXGI_ADAPTER_DESC*);
    typedef HRESULT (STDMETHODCALLTYPE *PFN_GetDesc1)(IDXGIAdapter1*, DXGI_ADAPTER_DESC1*);

    static PFN_GetDesc   g_origGetDesc   = nullptr;
    static PFN_GetDesc1  g_origGetDesc1  = nullptr;
    static bool          g_patchedGetDesc  = false;
    static bool          g_patchedGetDesc1 = false;

    // Auto-detect whether to apply the NVIDIA spoof. We only spoof when
    // the real hardware is NOT already NVIDIA. Reason: on a real NVIDIA
    // system NvApiProxy runs in PASSTHROUGH mode (real driver answers
    // NvAPI calls) and games querying DXGI for vendor get NVIDIA
    // naturally — no spoof needed. Worse, spoofing on NVIDIA changes the
    // CARD MODEL the game sees (to RTX 2080 Ti), and 3D Vision Auto Mode
    // games (Hitman Absolution, DiRT Rally) take this as a signal to
    // engage their full Auto-Mode stereo pipeline against a driver that
    // doesn't actually have 3D Vision Auto on a modern setup → crash.
    //
    // On non-NVIDIA hardware (AMD/Intel) the spoof IS essential because
    // games skip loading nvapi.dll if vendor isn't 0x10DE — without the
    // spoof, the entire stereo path never engages.
    static bool          g_shouldSpoof   = true;   // safe default; flipped at install

    void ApplySpoofToDesc(DXGI_ADAPTER_DESC* d)
    {
        if (!d) return;
        wcsncpy_s(d->Description, sizeof(d->Description) / sizeof(WCHAR),
                  kSpoofGpuNameW, _TRUNCATE);
        d->VendorId = kSpoofPciVendor;
        d->DeviceId = kSpoofPciDevice;
        d->SubSysId = kSpoofSubSysId;
        d->Revision = kSpoofRevision;
    }
    void ApplySpoofToDesc1(DXGI_ADAPTER_DESC1* d)
    {
        if (!d) return;
        wcsncpy_s(d->Description, sizeof(d->Description) / sizeof(WCHAR),
                  kSpoofGpuNameW, _TRUNCATE);
        d->VendorId = kSpoofPciVendor;
        d->DeviceId = kSpoofPciDevice;
        d->SubSysId = kSpoofSubSysId;
        d->Revision = kSpoofRevision;
        // Leave d->Flags alone (DXGI_ADAPTER_FLAG_SOFTWARE matters for
        // WARP detection — game might bail if we lie that a real adapter
        // is software).
    }

    HRESULT STDMETHODCALLTYPE Hook_GetDesc(IDXGIAdapter* pThis, DXGI_ADAPTER_DESC* pDesc)
    {
        if (!g_origGetDesc) return E_FAIL;
        HRESULT hr = g_origGetDesc(pThis, pDesc);
        if (SUCCEEDED(hr) && pDesc && g_shouldSpoof)
        {
            ApplySpoofToDesc(pDesc);
            static volatile LONG s_logged = 0;
            if (InterlockedCompareExchange(&s_logged, 1, 0) == 0)
                NvDM_DxgiLog("  Hook_GetDesc: first spoof fired -> NVIDIA RTX 2080 Ti (vendor=0x%04X)\n",
                             pDesc->VendorId);
        }
        return hr;
    }

    HRESULT STDMETHODCALLTYPE Hook_GetDesc1(IDXGIAdapter1* pThis, DXGI_ADAPTER_DESC1* pDesc)
    {
        if (!g_origGetDesc1) return E_FAIL;
        HRESULT hr = g_origGetDesc1(pThis, pDesc);
        if (SUCCEEDED(hr) && pDesc && g_shouldSpoof)
        {
            ApplySpoofToDesc1(pDesc);
            static volatile LONG s_logged = 0;
            if (InterlockedCompareExchange(&s_logged, 1, 0) == 0)
                NvDM_DxgiLog("  Hook_GetDesc1: first spoof fired -> NVIDIA RTX 2080 Ti (vendor=0x%04X)\n",
                             pDesc->VendorId);
        }
        return hr;
    }

    bool PatchVtableSlot(void** vtable, int slot, void* hook, void** outOriginal, const char* tag)
    {
        DWORD oldProtect = 0;
        if (!VirtualProtect(&vtable[slot], sizeof(void*), PAGE_READWRITE, &oldProtect))
        {
            NvDM_DxgiLog("  AdapterVtablePatch[%s]: VirtualProtect RW failed err=%lu\n",
                         tag, GetLastError());
            return false;
        }
        *outOriginal  = vtable[slot];
        vtable[slot]  = hook;
        DWORD restored = 0;
        VirtualProtect(&vtable[slot], sizeof(void*), oldProtect, &restored);
        NvDM_DxgiLog("  AdapterVtablePatch[%s] slot=%d: orig=%p -> hook=%p\n",
                     tag, slot, *outOriginal, hook);
        return true;
    }
} // anonymous namespace

bool InstallAdapterVtablePatch(IDXGIFactory* realFactory)
{
    if (g_patchedGetDesc && g_patchedGetDesc1) return true;
    if (!realFactory) return false;

    // Grab adapter 0 just to get the vtable pointer. The vtable is
    // shared across all IDXGIAdapter instances in this process so one
    // patch covers everyone.
    IDXGIAdapter* realAdapter = nullptr;
    HRESULT hr = realFactory->EnumAdapters(0, &realAdapter);
    if (FAILED(hr) || !realAdapter)
    {
        NvDM_DxgiLog("  InstallAdapterVtablePatch: EnumAdapters(0) failed hr=0x%08lX — adapter spoof disabled\n", hr);
        return false;
    }

    // Auto-detect: only enable the spoof if the real hardware ISN'T
    // already NVIDIA. On real NVIDIA the spoof is unnecessary (vendor
    // already matches what games expect) and counterproductive
    // (3D Vision Auto Mode games — Hitman, Dirt Rally — engage their
    // full Auto pipeline against a real driver that doesn't have 3D
    // Vision Auto on modern setups, and crash). On AMD/Intel the spoof
    // is essential — without it games skip loading nvapi.dll and the
    // entire stereo path never engages.
    DXGI_ADAPTER_DESC realDesc = {};
    if (SUCCEEDED(realAdapter->GetDesc(&realDesc)))
    {
        g_shouldSpoof = (realDesc.VendorId != kSpoofPciVendor);
        NvDM_DxgiLog("  InstallAdapterVtablePatch: real vendor=0x%04X, spoof=%d\n",
                     realDesc.VendorId, (int)g_shouldSpoof);
    }

    if (!g_patchedGetDesc)
    {
        void** vtable = *reinterpret_cast<void***>(realAdapter);
        if (PatchVtableSlot(vtable, kSlotAdapterGetDesc,
                             reinterpret_cast<void*>(&Hook_GetDesc),
                             reinterpret_cast<void**>(&g_origGetDesc),
                             "GetDesc"))
            g_patchedGetDesc = true;
    }

    if (!g_patchedGetDesc1)
    {
        IDXGIAdapter1* adapter1 = nullptr;
        if (SUCCEEDED(realAdapter->QueryInterface(IID_IDXGIAdapter1,
                                                   reinterpret_cast<void**>(&adapter1))) && adapter1)
        {
            void** vt1 = *reinterpret_cast<void***>(adapter1);
            if (PatchVtableSlot(vt1, kSlotAdapter1GetDesc1,
                                 reinterpret_cast<void*>(&Hook_GetDesc1),
                                 reinterpret_cast<void**>(&g_origGetDesc1),
                                 "GetDesc1"))
                g_patchedGetDesc1 = true;
            adapter1->Release();
        }
    }

    realAdapter->Release();
    return g_patchedGetDesc;
}

} // namespace NvDirectMode
