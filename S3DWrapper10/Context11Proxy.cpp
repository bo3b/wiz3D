/* wiz3D - ID3D11DeviceContext proxy implementation (Option B Stage 2)
 *
 * Pure passthrough port of NvDirectMode/d3d11/Context11Proxy. The stage-3 BB
 * tracking and stage-4 magic-header capture were stripped for the MVP — the
 * job here is to prove COM identity + refcounting are right, not to do any
 * stereo work yet. OMSet/RSSetViewports/CopyResource/CopySubresourceRegion
 * are forwarded unchanged; per-eye behaviour will be re-added in Stage 4.
 */

#include "StdAfx.h"
#include "Context11Proxy.h"
#include "Device11Proxy.h"
#include "Texture2D11Proxy.h"
#include "RTV11Proxy.h"
#include "DSV11Proxy.h"
#include "proxy_factory.h"     // TryUnwrap* helpers
#include "AdapterFunctions.h"  // DDILog

// Static-size cap on per-call temp arrays used to unwrap RTV/RSV pointer
// arrays passed to OMSetRenderTargets. D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT
// is 8; UAVs go higher but we cap defensively.
static constexpr UINT kMaxUnwrapArray = 16;

#pragma comment(lib, "dxguid.lib")

namespace wiz3d
{

Context11Proxy::Context11Proxy(ID3D11DeviceContext* real, Device11Proxy* parent)
    : m_real(real)
    , m_parent(parent)
    , m_refs(1)
    , m_currentBBBound(false)
    , m_activeEye(Eye::Left)
    , m_presentHookActive(false)
{
}

Context11Proxy::~Context11Proxy()
{
    // Closures hold no AddRef'd state in 4b.1 (Draw/DrawIndexed capture only
    // POD args), so a plain clear is safe. Later stages that record state-
    // setting calls with captured COM pointers will release them in
    // ClearFrameCommands; the dtor will route there.
    ClearFrameCommands();
}

void Context11Proxy::ClearFrameCommands()
{
    m_frameCommands.clear();
}

void Context11Proxy::ReplayFrameCommands(Eye eye)
{
    // Snapshot + flip the active eye for the replay pass. Each recorded
    // closure re-enters our proxy methods, so OMSet/etc. pick the
    // eye-appropriate real handle via m_activeEye automatically.
    Eye saved = m_activeEye;
    m_activeEye = eye;
    for (auto& fn : m_frameCommands)
        fn();
    m_activeEye = saved;
}

// Stage 4b.4 (more state setters): record-and-replay for *SetShaderResources
// across all 6 shader stages. Each stage's method body is identical except
// for the method name, so a macro keeps the boilerplate tractable. SRVs are
// not yet wrapped (Stage 3c) so capture-and-restore is straightforward —
// just hold a ref through the lambda's lifetime so the SRV survives the
// frame even if the game releases it. Same gate (m_presentHookActive) as
// OMSet so games whose swap chain bypasses us stay safely in passthrough.
#define RECORD_SRV_SET(STAGE_PREFIX)                                                        \
void STDMETHODCALLTYPE Context11Proxy::STAGE_PREFIX##SetShaderResources(                    \
    UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView* const* ppShaderResourceViews)  \
{                                                                                           \
    m_real->STAGE_PREFIX##SetShaderResources(StartSlot, NumViews, ppShaderResourceViews);   \
    if (!m_presentHookActive) return;                                                       \
    std::vector<ComRefHolder> refs;                                                         \
    refs.reserve(NumViews);                                                                 \
    for (UINT i = 0; i < NumViews; ++i)                                                     \
        refs.emplace_back(ppShaderResourceViews ? ppShaderResourceViews[i] : nullptr);      \
    m_frameCommands.emplace_back(                                                           \
        [this, StartSlot, NumViews, refs]() {                                               \
            ID3D11ShaderResourceView* raw[kMaxUnwrapArray] = { 0 };                         \
            UINT cap = NumViews <= kMaxUnwrapArray ? NumViews : kMaxUnwrapArray;            \
            for (UINT i = 0; i < cap; ++i)                                                  \
                raw[i] = static_cast<ID3D11ShaderResourceView*>(refs[i].p);                 \
            m_real->STAGE_PREFIX##SetShaderResources(StartSlot, NumViews, raw);             \
        });                                                                                 \
}
RECORD_SRV_SET(VS)
RECORD_SRV_SET(PS)
RECORD_SRV_SET(GS)
RECORD_SRV_SET(HS)
RECORD_SRV_SET(DS)
RECORD_SRV_SET(CS)
#undef RECORD_SRV_SET

// *SetSamplers — same shape as *SetShaderResources but with ID3D11SamplerState.
#define RECORD_SAMPLER_SET(STAGE_PREFIX)                                                    \
void STDMETHODCALLTYPE Context11Proxy::STAGE_PREFIX##SetSamplers(                           \
    UINT StartSlot, UINT NumSamplers, ID3D11SamplerState* const* ppSamplers)                \
{                                                                                           \
    m_real->STAGE_PREFIX##SetSamplers(StartSlot, NumSamplers, ppSamplers);                  \
    if (!m_presentHookActive) return;                                                       \
    std::vector<ComRefHolder> refs;                                                         \
    refs.reserve(NumSamplers);                                                              \
    for (UINT i = 0; i < NumSamplers; ++i)                                                  \
        refs.emplace_back(ppSamplers ? ppSamplers[i] : nullptr);                            \
    m_frameCommands.emplace_back(                                                           \
        [this, StartSlot, NumSamplers, refs]() {                                            \
            ID3D11SamplerState* raw[kMaxUnwrapArray] = { 0 };                               \
            UINT cap = NumSamplers <= kMaxUnwrapArray ? NumSamplers : kMaxUnwrapArray;      \
            for (UINT i = 0; i < cap; ++i)                                                  \
                raw[i] = static_cast<ID3D11SamplerState*>(refs[i].p);                       \
            m_real->STAGE_PREFIX##SetSamplers(StartSlot, NumSamplers, raw);                 \
        });                                                                                 \
}
RECORD_SAMPLER_SET(VS)
RECORD_SAMPLER_SET(PS)
RECORD_SAMPLER_SET(GS)
RECORD_SAMPLER_SET(HS)
RECORD_SAMPLER_SET(DS)
RECORD_SAMPLER_SET(CS)
#undef RECORD_SAMPLER_SET

// *SetConstantBuffers — same shape with ID3D11Buffer. Stage 4c will modify
// the closure body to apply per-eye CB writes (left-right view projection
// matrix), but for 4b.4 it's straight passthrough record-and-replay.
#define RECORD_CB_SET(STAGE_PREFIX)                                                         \
void STDMETHODCALLTYPE Context11Proxy::STAGE_PREFIX##SetConstantBuffers(                    \
    UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppConstantBuffers)                \
{                                                                                           \
    m_real->STAGE_PREFIX##SetConstantBuffers(StartSlot, NumBuffers, ppConstantBuffers);     \
    if (!m_presentHookActive) return;                                                       \
    std::vector<ComRefHolder> refs;                                                         \
    refs.reserve(NumBuffers);                                                               \
    for (UINT i = 0; i < NumBuffers; ++i)                                                   \
        refs.emplace_back(ppConstantBuffers ? ppConstantBuffers[i] : nullptr);              \
    m_frameCommands.emplace_back(                                                           \
        [this, StartSlot, NumBuffers, refs]() {                                             \
            ID3D11Buffer* raw[kMaxUnwrapArray] = { 0 };                                     \
            UINT cap = NumBuffers <= kMaxUnwrapArray ? NumBuffers : kMaxUnwrapArray;        \
            for (UINT i = 0; i < cap; ++i)                                                  \
                raw[i] = static_cast<ID3D11Buffer*>(refs[i].p);                             \
            m_real->STAGE_PREFIX##SetConstantBuffers(StartSlot, NumBuffers, raw);           \
        });                                                                                 \
}
RECORD_CB_SET(VS)
RECORD_CB_SET(PS)
RECORD_CB_SET(GS)
RECORD_CB_SET(HS)
RECORD_CB_SET(DS)
RECORD_CB_SET(CS)
#undef RECORD_CB_SET

// *SetShader — takes the stage-specific shader interface plus the
// class-instance array. Class instances are rarely non-null (used for
// dynamic shader linking) but the array is captured for fidelity.
#define RECORD_SHADER_SET(STAGE_PREFIX, SHADER_TYPE)                                        \
void STDMETHODCALLTYPE Context11Proxy::STAGE_PREFIX##SetShader(                             \
    SHADER_TYPE* pShader, ID3D11ClassInstance* const* ppClassInstances, UINT NumClassInstances) \
{                                                                                           \
    m_real->STAGE_PREFIX##SetShader(pShader, ppClassInstances, NumClassInstances);          \
    if (!m_presentHookActive) return;                                                       \
    ComRefHolder shaderRef(pShader);                                                        \
    std::vector<ComRefHolder> ciRefs;                                                       \
    ciRefs.reserve(NumClassInstances);                                                      \
    for (UINT i = 0; i < NumClassInstances; ++i)                                            \
        ciRefs.emplace_back(ppClassInstances ? ppClassInstances[i] : nullptr);              \
    m_frameCommands.emplace_back(                                                           \
        [this, shaderRef, ciRefs, NumClassInstances]() {                                    \
            ID3D11ClassInstance* raw[kMaxUnwrapArray] = { 0 };                              \
            UINT cap = NumClassInstances <= kMaxUnwrapArray ? NumClassInstances : kMaxUnwrapArray; \
            for (UINT i = 0; i < cap; ++i)                                                  \
                raw[i] = static_cast<ID3D11ClassInstance*>(ciRefs[i].p);                    \
            m_real->STAGE_PREFIX##SetShader(                                                \
                static_cast<SHADER_TYPE*>(shaderRef.p),                                     \
                ciRefs.empty() ? nullptr : raw,                                             \
                NumClassInstances);                                                         \
        });                                                                                 \
}
RECORD_SHADER_SET(VS, ID3D11VertexShader)
RECORD_SHADER_SET(PS, ID3D11PixelShader)
RECORD_SHADER_SET(GS, ID3D11GeometryShader)
RECORD_SHADER_SET(HS, ID3D11HullShader)
RECORD_SHADER_SET(DS, ID3D11DomainShader)
RECORD_SHADER_SET(CS, ID3D11ComputeShader)
#undef RECORD_SHADER_SET

// Stage 4b.7: record-and-replay for draw/dispatch. Pure POD captures for the
// non-Indirect variants; Indirect/Dispatch-with-buffer use ComRefHolder to
// keep the arg buffer alive across replay. The closure body is the same shape
// as the original passthrough — no Do* helpers needed since draws don't
// reference our wrapped resources directly (the bound RTV/VB/IB/CB are picked
// up from the bound pipeline state, which the *Set* closures already replay
// with eye selection).

void STDMETHODCALLTYPE Context11Proxy::Draw(UINT VertexCount, UINT StartVertexLocation)
{
    m_real->Draw(VertexCount, StartVertexLocation);
    if (!m_presentHookActive) return;
    m_frameCommands.emplace_back(
        [this, VertexCount, StartVertexLocation]()
        {
            m_real->Draw(VertexCount, StartVertexLocation);
        });
}

void STDMETHODCALLTYPE Context11Proxy::DrawIndexed(
    UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    m_real->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
    if (!m_presentHookActive) return;
    m_frameCommands.emplace_back(
        [this, IndexCount, StartIndexLocation, BaseVertexLocation]()
        {
            m_real->DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation);
        });
}

void STDMETHODCALLTYPE Context11Proxy::DrawInstanced(
    UINT VertexCountPerInstance, UINT InstanceCount,
    UINT StartVertexLocation, UINT StartInstanceLocation)
{
    m_real->DrawInstanced(VertexCountPerInstance, InstanceCount,
                          StartVertexLocation, StartInstanceLocation);
    if (!m_presentHookActive) return;
    m_frameCommands.emplace_back(
        [this, VertexCountPerInstance, InstanceCount,
         StartVertexLocation, StartInstanceLocation]()
        {
            m_real->DrawInstanced(VertexCountPerInstance, InstanceCount,
                                   StartVertexLocation, StartInstanceLocation);
        });
}

void STDMETHODCALLTYPE Context11Proxy::DrawIndexedInstanced(
    UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
    INT BaseVertexLocation, UINT StartInstanceLocation)
{
    m_real->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount,
                                  StartIndexLocation, BaseVertexLocation,
                                  StartInstanceLocation);
    if (!m_presentHookActive) return;
    m_frameCommands.emplace_back(
        [this, IndexCountPerInstance, InstanceCount, StartIndexLocation,
         BaseVertexLocation, StartInstanceLocation]()
        {
            m_real->DrawIndexedInstanced(
                IndexCountPerInstance, InstanceCount, StartIndexLocation,
                BaseVertexLocation, StartInstanceLocation);
        });
}

void STDMETHODCALLTYPE Context11Proxy::DrawAuto()
{
    m_real->DrawAuto();
    if (!m_presentHookActive) return;
    m_frameCommands.emplace_back(
        [this]() { m_real->DrawAuto(); });
}

void STDMETHODCALLTYPE Context11Proxy::DrawInstancedIndirect(
    ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
    m_real->DrawInstancedIndirect(pBufferForArgs, AlignedByteOffsetForArgs);
    if (!m_presentHookActive) return;
    ComRefHolder bufRef(pBufferForArgs);
    m_frameCommands.emplace_back(
        [this, bufRef, AlignedByteOffsetForArgs]()
        {
            m_real->DrawInstancedIndirect(
                static_cast<ID3D11Buffer*>(bufRef.p), AlignedByteOffsetForArgs);
        });
}

void STDMETHODCALLTYPE Context11Proxy::DrawIndexedInstancedIndirect(
    ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
    m_real->DrawIndexedInstancedIndirect(pBufferForArgs, AlignedByteOffsetForArgs);
    if (!m_presentHookActive) return;
    ComRefHolder bufRef(pBufferForArgs);
    m_frameCommands.emplace_back(
        [this, bufRef, AlignedByteOffsetForArgs]()
        {
            m_real->DrawIndexedInstancedIndirect(
                static_cast<ID3D11Buffer*>(bufRef.p), AlignedByteOffsetForArgs);
        });
}

void STDMETHODCALLTYPE Context11Proxy::Dispatch(
    UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
    m_real->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
    if (!m_presentHookActive) return;
    m_frameCommands.emplace_back(
        [this, ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ]()
        {
            m_real->Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ);
        });
}

void STDMETHODCALLTYPE Context11Proxy::DispatchIndirect(
    ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
    m_real->DispatchIndirect(pBufferForArgs, AlignedByteOffsetForArgs);
    if (!m_presentHookActive) return;
    ComRefHolder bufRef(pBufferForArgs);
    m_frameCommands.emplace_back(
        [this, bufRef, AlignedByteOffsetForArgs]()
        {
            m_real->DispatchIndirect(
                static_cast<ID3D11Buffer*>(bufRef.p), AlignedByteOffsetForArgs);
        });
}

HRESULT STDMETHODCALLTYPE Context11Proxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_ID3D11DeviceChild ||
        riid == IID_ID3D11DeviceContext)
    {
        *ppvObj = static_cast<ID3D11DeviceContext*>(this);
        AddRef();
        return S_OK;
    }
    // Context1+ family: refuse so games fall back to the wrapped base
    // interface instead of getting an unwrapped escape hatch.
    *ppvObj = nullptr;
    return E_NOINTERFACE;
}

void Context11Proxy::DoOMSetRenderTargets(
    UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews,
    ID3D11DepthStencilView* pDepthStencilView)
{
    // Stage 4a: pick the left- or right-eye real handle for each wrapped
    // RTV/DSV based on m_activeEye. When the proxy isn't stereo, both
    // GetReal() and GetRealRight() resolve to the same left-eye handle (the
    // latter is null, so we fall back to left). Stage 4b.8 will flip
    // m_activeEye between L/R passes during the per-frame replay.
    bool pickRight = (m_activeEye == Eye::Right);
    ID3D11RenderTargetView* realRTVs[kMaxUnwrapArray] = { 0 };
    ID3D11RenderTargetView* const* rtvsToUse = ppRenderTargetViews;
    if (NumViews > 0 && ppRenderTargetViews)
    {
        UINT cap = NumViews <= kMaxUnwrapArray ? NumViews : kMaxUnwrapArray;
        for (UINT i = 0; i < cap; ++i)
        {
            RTV11Proxy* p = TryUnwrapRTV(ppRenderTargetViews[i]);
            if (!p)
            {
                realRTVs[i] = ppRenderTargetViews[i];
                continue;
            }
            ID3D11RenderTargetView* right = p->GetRealRight();
            realRTVs[i] = (pickRight && right) ? right : p->GetReal();
        }
        rtvsToUse = realRTVs;
    }
    ID3D11DepthStencilView* realDSV = pDepthStencilView;
    if (DSV11Proxy* d = TryUnwrapDSV(pDepthStencilView))
    {
        ID3D11DepthStencilView* right = d->GetRealRight();
        realDSV = (pickRight && right) ? right : d->GetReal();
    }
    m_real->OMSetRenderTargets(NumViews, rtvsToUse, realDSV);
}

void STDMETHODCALLTYPE Context11Proxy::OMSetRenderTargets(
    UINT NumViews, ID3D11RenderTargetView* const* ppRenderTargetViews,
    ID3D11DepthStencilView* pDepthStencilView)
{
    DoOMSetRenderTargets(NumViews, ppRenderTargetViews, pDepthStencilView);

    // Stage 4b.4: record-for-replay, but only when the Present hook is
    // active. Without a flush-each-frame trigger the vector would grow
    // unbounded, so games whose swap chain bypasses us stay safely in pure
    // passthrough mode. Capture the wrapped pointers by value (ComRefHolder
    // copy ctor AddRefs) so the lambda holds its own refs for the frame
    // even if the game releases. At replay time the closure re-calls
    // DoOMSetRenderTargets, which re-runs eye-aware unwrap with whatever
    // m_activeEye is set to at that point.
    if (!m_presentHookActive) return;
    std::vector<ComRefHolder> rtvRefs;
    rtvRefs.reserve(NumViews);
    for (UINT i = 0; i < NumViews; ++i)
        rtvRefs.emplace_back(ppRenderTargetViews ? ppRenderTargetViews[i] : nullptr);
    ComRefHolder dsvRef(pDepthStencilView);
    m_frameCommands.emplace_back(
        [this, NumViews, rtvRefs, dsvRef]()
        {
            // Rebuild raw-pointer array from the captured holders.
            ID3D11RenderTargetView* raw[kMaxUnwrapArray] = { 0 };
            UINT cap = NumViews <= kMaxUnwrapArray ? NumViews : kMaxUnwrapArray;
            for (UINT i = 0; i < cap; ++i)
                raw[i] = static_cast<ID3D11RenderTargetView*>(rtvRefs[i].p);
            DoOMSetRenderTargets(NumViews, raw,
                static_cast<ID3D11DepthStencilView*>(dsvRef.p));
        });
}

void Context11Proxy::DoOMSetRenderTargetsAndUnorderedAccessViews(
    UINT NumRTVs, ID3D11RenderTargetView* const* ppRenderTargetViews,
    ID3D11DepthStencilView* pDepthStencilView,
    UINT UAVStartSlot, UINT NumUAVs,
    ID3D11UnorderedAccessView* const* ppUnorderedAccessViews,
    const UINT* pUAVInitialCounts)
{
    bool pickRight = (m_activeEye == Eye::Right);
    ID3D11RenderTargetView* realRTVs[kMaxUnwrapArray] = { 0 };
    ID3D11RenderTargetView* const* rtvsToUse = ppRenderTargetViews;
    if (NumRTVs != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL &&
        NumRTVs > 0 && ppRenderTargetViews)
    {
        UINT cap = NumRTVs <= kMaxUnwrapArray ? NumRTVs : kMaxUnwrapArray;
        for (UINT i = 0; i < cap; ++i)
        {
            RTV11Proxy* p = TryUnwrapRTV(ppRenderTargetViews[i]);
            if (!p)
            {
                realRTVs[i] = ppRenderTargetViews[i];
                continue;
            }
            ID3D11RenderTargetView* right = p->GetRealRight();
            realRTVs[i] = (pickRight && right) ? right : p->GetReal();
        }
        rtvsToUse = realRTVs;
    }
    ID3D11DepthStencilView* realDSV = pDepthStencilView;
    if (NumRTVs != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL)
    {
        if (DSV11Proxy* d = TryUnwrapDSV(pDepthStencilView))
        {
            ID3D11DepthStencilView* right = d->GetRealRight();
            realDSV = (pickRight && right) ? right : d->GetReal();
        }
    }
    // UAVs not yet wrapped (Stage 3c). Pass through unchanged.
    m_real->OMSetRenderTargetsAndUnorderedAccessViews(
        NumRTVs, rtvsToUse, realDSV,
        UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
}

void STDMETHODCALLTYPE Context11Proxy::OMSetRenderTargetsAndUnorderedAccessViews(
    UINT NumRTVs, ID3D11RenderTargetView* const* ppRenderTargetViews,
    ID3D11DepthStencilView* pDepthStencilView,
    UINT UAVStartSlot, UINT NumUAVs,
    ID3D11UnorderedAccessView* const* ppUnorderedAccessViews,
    const UINT* pUAVInitialCounts)
{
    DoOMSetRenderTargetsAndUnorderedAccessViews(
        NumRTVs, ppRenderTargetViews, pDepthStencilView,
        UAVStartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);

    if (!m_presentHookActive) return;
    // Stage 4b.4: record. Both RTV and UAV arrays need capture.
    std::vector<ComRefHolder> rtvRefs;
    if (NumRTVs != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL && ppRenderTargetViews)
    {
        rtvRefs.reserve(NumRTVs);
        for (UINT i = 0; i < NumRTVs; ++i)
            rtvRefs.emplace_back(ppRenderTargetViews[i]);
    }
    ComRefHolder dsvRef(pDepthStencilView);

    std::vector<ComRefHolder> uavRefs;
    if (ppUnorderedAccessViews)
    {
        uavRefs.reserve(NumUAVs);
        for (UINT i = 0; i < NumUAVs; ++i)
            uavRefs.emplace_back(ppUnorderedAccessViews[i]);
    }
    std::vector<UINT> initialCounts;
    if (pUAVInitialCounts)
        initialCounts.assign(pUAVInitialCounts, pUAVInitialCounts + NumUAVs);

    m_frameCommands.emplace_back(
        [this, NumRTVs, rtvRefs, dsvRef,
         UAVStartSlot, NumUAVs, uavRefs, initialCounts]()
        {
            ID3D11RenderTargetView* rawRTVs[kMaxUnwrapArray] = { 0 };
            ID3D11RenderTargetView* const* rtvArg = nullptr;
            if (NumRTVs != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL && !rtvRefs.empty())
            {
                UINT cap = NumRTVs <= kMaxUnwrapArray ? NumRTVs : kMaxUnwrapArray;
                for (UINT i = 0; i < cap; ++i)
                    rawRTVs[i] = static_cast<ID3D11RenderTargetView*>(rtvRefs[i].p);
                rtvArg = rawRTVs;
            }
            // UAVs reconstructed similarly (capped to a separate stack array
            // since the D3D11 UAV slot count goes beyond kMaxUnwrapArray;
            // we use the same cap value defensively).
            ID3D11UnorderedAccessView* rawUAVs[kMaxUnwrapArray] = { 0 };
            ID3D11UnorderedAccessView* const* uavArg = nullptr;
            if (!uavRefs.empty())
            {
                UINT cap = NumUAVs <= kMaxUnwrapArray ? NumUAVs : kMaxUnwrapArray;
                for (UINT i = 0; i < cap; ++i)
                    rawUAVs[i] = static_cast<ID3D11UnorderedAccessView*>(uavRefs[i].p);
                uavArg = rawUAVs;
            }
            const UINT* countsArg = initialCounts.empty() ? nullptr : initialCounts.data();
            DoOMSetRenderTargetsAndUnorderedAccessViews(
                NumRTVs, rtvArg,
                static_cast<ID3D11DepthStencilView*>(dsvRef.p),
                UAVStartSlot, NumUAVs, uavArg, countsArg);
        });
}

void STDMETHODCALLTYPE Context11Proxy::RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT* pViewports)
{
    m_real->RSSetViewports(NumViewports, pViewports);
}

void Context11Proxy::DoCopyResource(
    ID3D11Resource* pDstResource, ID3D11Resource* pSrcResource)
{
    bool pickRight = (m_activeEye == Eye::Right);
    Texture2D11Proxy* dst = TryUnwrapTexture2D(pDstResource);
    Texture2D11Proxy* src = TryUnwrapTexture2D(pSrcResource);
    ID3D11Resource* realDst = pDstResource;
    if (dst)
    {
        ID3D11Resource* right = dst->GetRealRight();
        realDst = (pickRight && right) ? right : dst->GetReal();
    }
    ID3D11Resource* realSrc = pSrcResource;
    if (src)
    {
        ID3D11Resource* right = src->GetRealRight();
        realSrc = (pickRight && right) ? right : src->GetReal();
    }
    m_real->CopyResource(realDst, realSrc);
}

void STDMETHODCALLTYPE Context11Proxy::CopyResource(
    ID3D11Resource* pDstResource, ID3D11Resource* pSrcResource)
{
    DoCopyResource(pDstResource, pSrcResource);
    if (!m_presentHookActive) return;
    ComRefHolder dstRef(pDstResource);
    ComRefHolder srcRef(pSrcResource);
    m_frameCommands.emplace_back(
        [this, dstRef, srcRef]()
        {
            DoCopyResource(static_cast<ID3D11Resource*>(dstRef.p),
                           static_cast<ID3D11Resource*>(srcRef.p));
        });
}

void Context11Proxy::DoCopySubresourceRegion(
    ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY,
    UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource,
    const D3D11_BOX* pSrcBox)
{
    bool pickRight = (m_activeEye == Eye::Right);
    Texture2D11Proxy* dst = TryUnwrapTexture2D(pDstResource);
    Texture2D11Proxy* src = TryUnwrapTexture2D(pSrcResource);
    ID3D11Resource* realDst = pDstResource;
    if (dst)
    {
        ID3D11Resource* right = dst->GetRealRight();
        realDst = (pickRight && right) ? right : dst->GetReal();
    }
    ID3D11Resource* realSrc = pSrcResource;
    if (src)
    {
        ID3D11Resource* right = src->GetRealRight();
        realSrc = (pickRight && right) ? right : src->GetReal();
    }
    m_real->CopySubresourceRegion(
        realDst, DstSubresource, DstX, DstY, DstZ,
        realSrc, SrcSubresource, pSrcBox);
}

void STDMETHODCALLTYPE Context11Proxy::CopySubresourceRegion(
    ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY,
    UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource,
    const D3D11_BOX* pSrcBox)
{
    DoCopySubresourceRegion(pDstResource, DstSubresource, DstX, DstY, DstZ,
                            pSrcResource, SrcSubresource, pSrcBox);
    if (!m_presentHookActive) return;
    ComRefHolder dstRef(pDstResource);
    ComRefHolder srcRef(pSrcResource);
    bool hasBox = (pSrcBox != nullptr);
    D3D11_BOX box = {};
    if (hasBox) box = *pSrcBox;
    m_frameCommands.emplace_back(
        [this, dstRef, DstSubresource, DstX, DstY, DstZ,
         srcRef, SrcSubresource, hasBox, box]()
        {
            DoCopySubresourceRegion(
                static_cast<ID3D11Resource*>(dstRef.p),
                DstSubresource, DstX, DstY, DstZ,
                static_cast<ID3D11Resource*>(srcRef.p),
                SrcSubresource, hasBox ? &box : nullptr);
        });
}

HRESULT STDMETHODCALLTYPE Context11Proxy::Map(
    ID3D11Resource* pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags,
    D3D11_MAPPED_SUBRESOURCE* pMappedResource)
{
    Texture2D11Proxy* tex = TryUnwrapTexture2D(pResource);
    ID3D11Resource* realRes = tex ? tex->GetReal() : pResource;
    HRESULT hr = m_real->Map(realRes, Subresource, MapType, MapFlags, pMappedResource);
    if (FAILED(hr) || !pMappedResource) return hr;
    if (!m_presentHookActive) return hr;

    // Stage 4b.5: only record write maps — read maps can't be meaningfully
    // replayed because the game's pointer becomes stale after our snapshot.
    if (MapType != D3D11_MAP_WRITE &&
        MapType != D3D11_MAP_WRITE_DISCARD &&
        MapType != D3D11_MAP_WRITE_NO_OVERWRITE &&
        MapType != D3D11_MAP_READ_WRITE)
        return hr;

    // Buffers only for 4b.5 — texture replay needs row/depth pitch and array-
    // slice metadata which is a Stage 4b.6 concern. Constant-buffer Map() is
    // the only mapping path our test corpus exercises in the per-eye loop, and
    // CBs are all buffers, so this gets us full 4c coverage.
    ID3D11Buffer* asBuffer = nullptr;
    if (FAILED(pResource->QueryInterface(__uuidof(ID3D11Buffer),
                                          reinterpret_cast<void**>(&asBuffer))) || !asBuffer)
        return hr;
    D3D11_BUFFER_DESC desc;
    asBuffer->GetDesc(&desc);
    asBuffer->Release();
    if (desc.ByteWidth == 0) return hr;

    ActiveMap am;
    am.resource    = pResource;
    am.subresource = Subresource;
    am.mapType     = MapType;
    am.mappedData  = pMappedResource->pData;
    am.byteWidth   = desc.ByteWidth;
    m_activeMaps.push_back(am);
    return hr;
}

void STDMETHODCALLTYPE Context11Proxy::Unmap(ID3D11Resource* pResource, UINT Subresource)
{
    Texture2D11Proxy* tex = TryUnwrapTexture2D(pResource);
    ID3D11Resource* realRes = tex ? tex->GetReal() : pResource;

    // Stage 4b.5: if we recorded the Map, snapshot the bytes the game wrote
    // BEFORE forwarding Unmap (which invalidates the mapped pointer), then
    // push a closure that re-issues Map → memcpy → Unmap at replay time.
    // Stage 4c will hook this closure to apply per-eye CB modifications
    // between the memcpy and the Unmap.
    for (auto it = m_activeMaps.begin(); it != m_activeMaps.end(); ++it)
    {
        if (it->resource != pResource || it->subresource != Subresource) continue;
        if (m_presentHookActive && it->mappedData && it->byteWidth)
        {
            std::vector<unsigned char> bytes(it->byteWidth);
            memcpy(bytes.data(), it->mappedData, it->byteWidth);
            UINT subres = it->subresource;
            D3D11_MAP mapType = it->mapType;
            ComRefHolder resRef(pResource);
            m_frameCommands.emplace_back(
                [this, resRef, subres, bytes, mapType]()
                {
                    auto* gameRes = static_cast<ID3D11Resource*>(resRef.p);
                    Texture2D11Proxy* texR = TryUnwrapTexture2D(gameRes);
                    ID3D11Resource* real = texR ? texR->GetReal() : gameRes;
                    D3D11_MAPPED_SUBRESOURCE mapped = {};
                    if (SUCCEEDED(m_real->Map(real, subres, mapType, 0, &mapped))
                        && mapped.pData)
                    {
                        memcpy(mapped.pData, bytes.data(), bytes.size());
                        m_real->Unmap(real, subres);
                    }
                });
        }
        m_activeMaps.erase(it);
        break;
    }

    m_real->Unmap(realRes, Subresource);
}

void Context11Proxy::DoUpdateSubresource(
    ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox,
    const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
    bool pickRight = (m_activeEye == Eye::Right);
    Texture2D11Proxy* tex = TryUnwrapTexture2D(pDstResource);
    ID3D11Resource* real = pDstResource;
    if (tex)
    {
        ID3D11Resource* right = tex->GetRealRight();
        real = (pickRight && right) ? right : tex->GetReal();
    }
    m_real->UpdateSubresource(real, DstSubresource, pDstBox,
                              pSrcData, SrcRowPitch, SrcDepthPitch);
}

void STDMETHODCALLTYPE Context11Proxy::UpdateSubresource(
    ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox,
    const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
    DoUpdateSubresource(pDstResource, DstSubresource, pDstBox,
                        pSrcData, SrcRowPitch, SrcDepthPitch);
    if (!m_presentHookActive) return;

    // Snapshot pSrcData — the caller owns it and can free after the call.
    // For buffers we need ByteWidth; for textures we use SrcDepthPitch (or
    // SrcRowPitch if depth pitch is zero, for 2D textures) as an upper bound.
    UINT bytes = 0;
    ID3D11Buffer* asBuffer = nullptr;
    if (SUCCEEDED(pDstResource->QueryInterface(__uuidof(ID3D11Buffer),
                                                reinterpret_cast<void**>(&asBuffer))) && asBuffer)
    {
        D3D11_BUFFER_DESC desc;
        asBuffer->GetDesc(&desc);
        bytes = pDstBox
                ? (pDstBox->right - pDstBox->left)
                : desc.ByteWidth;
        asBuffer->Release();
    }
    else if (SrcDepthPitch)
    {
        bytes = SrcDepthPitch;
    }
    else if (SrcRowPitch)
    {
        // 2D texture row pitch × height — height we don't know without GetDesc.
        // For 4b.6 scope, cap at SrcRowPitch alone if we can't query height;
        // texture UpdateSubresource is rare in the per-eye loop.
        bytes = SrcRowPitch;
    }

    std::vector<unsigned char> data;
    if (bytes && pSrcData)
    {
        data.resize(bytes);
        memcpy(data.data(), pSrcData, bytes);
    }
    bool hasBox = (pDstBox != nullptr);
    D3D11_BOX box = {};
    if (hasBox) box = *pDstBox;
    ComRefHolder dstRef(pDstResource);
    m_frameCommands.emplace_back(
        [this, dstRef, DstSubresource, hasBox, box,
         data, SrcRowPitch, SrcDepthPitch]()
        {
            DoUpdateSubresource(
                static_cast<ID3D11Resource*>(dstRef.p),
                DstSubresource, hasBox ? &box : nullptr,
                data.empty() ? nullptr : data.data(),
                SrcRowPitch, SrcDepthPitch);
        });
}

void Context11Proxy::DoResolveSubresource(
    ID3D11Resource* pDstResource, UINT DstSubresource,
    ID3D11Resource* pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format)
{
    bool pickRight = (m_activeEye == Eye::Right);
    Texture2D11Proxy* dst = TryUnwrapTexture2D(pDstResource);
    Texture2D11Proxy* src = TryUnwrapTexture2D(pSrcResource);
    ID3D11Resource* realDst = pDstResource;
    if (dst)
    {
        ID3D11Resource* right = dst->GetRealRight();
        realDst = (pickRight && right) ? right : dst->GetReal();
    }
    ID3D11Resource* realSrc = pSrcResource;
    if (src)
    {
        ID3D11Resource* right = src->GetRealRight();
        realSrc = (pickRight && right) ? right : src->GetReal();
    }
    m_real->ResolveSubresource(realDst, DstSubresource,
                                realSrc, SrcSubresource, Format);
}

void STDMETHODCALLTYPE Context11Proxy::ResolveSubresource(
    ID3D11Resource* pDstResource, UINT DstSubresource,
    ID3D11Resource* pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format)
{
    DoResolveSubresource(pDstResource, DstSubresource,
                         pSrcResource, SrcSubresource, Format);
    if (!m_presentHookActive) return;
    ComRefHolder dstRef(pDstResource);
    ComRefHolder srcRef(pSrcResource);
    m_frameCommands.emplace_back(
        [this, dstRef, DstSubresource, srcRef, SrcSubresource, Format]()
        {
            DoResolveSubresource(
                static_cast<ID3D11Resource*>(dstRef.p), DstSubresource,
                static_cast<ID3D11Resource*>(srcRef.p), SrcSubresource, Format);
        });
}

void Context11Proxy::DoClearRenderTargetView(
    ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4])
{
    bool pickRight = (m_activeEye == Eye::Right);
    RTV11Proxy* rtv = TryUnwrapRTV(pRenderTargetView);
    ID3D11RenderTargetView* real = pRenderTargetView;
    if (rtv)
    {
        ID3D11RenderTargetView* right = rtv->GetRealRight();
        real = (pickRight && right) ? right : rtv->GetReal();
    }
    m_real->ClearRenderTargetView(real, ColorRGBA);
}

void STDMETHODCALLTYPE Context11Proxy::ClearRenderTargetView(
    ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4])
{
    DoClearRenderTargetView(pRenderTargetView, ColorRGBA);
    if (!m_presentHookActive) return;
    ComRefHolder rtvRef(pRenderTargetView);
    FLOAT color[4] = { 0, 0, 0, 0 };
    if (ColorRGBA)
    {
        color[0] = ColorRGBA[0]; color[1] = ColorRGBA[1];
        color[2] = ColorRGBA[2]; color[3] = ColorRGBA[3];
    }
    m_frameCommands.emplace_back(
        [this, rtvRef, color]()
        {
            DoClearRenderTargetView(
                static_cast<ID3D11RenderTargetView*>(rtvRef.p), color);
        });
}

void Context11Proxy::DoClearDepthStencilView(
    ID3D11DepthStencilView* pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
    bool pickRight = (m_activeEye == Eye::Right);
    DSV11Proxy* dsv = TryUnwrapDSV(pDepthStencilView);
    ID3D11DepthStencilView* real = pDepthStencilView;
    if (dsv)
    {
        ID3D11DepthStencilView* right = dsv->GetRealRight();
        real = (pickRight && right) ? right : dsv->GetReal();
    }
    m_real->ClearDepthStencilView(real, ClearFlags, Depth, Stencil);
}

void STDMETHODCALLTYPE Context11Proxy::ClearDepthStencilView(
    ID3D11DepthStencilView* pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
    DoClearDepthStencilView(pDepthStencilView, ClearFlags, Depth, Stencil);
    if (!m_presentHookActive) return;
    ComRefHolder dsvRef(pDepthStencilView);
    m_frameCommands.emplace_back(
        [this, dsvRef, ClearFlags, Depth, Stencil]()
        {
            DoClearDepthStencilView(
                static_cast<ID3D11DepthStencilView*>(dsvRef.p),
                ClearFlags, Depth, Stencil);
        });
}

// Stage 4b.4 Group C: remaining state setters. Same record-and-replay pattern
// as the macro-generated groups above, but each has a slightly different
// argument shape so they're written out individually. All gated on
// m_presentHookActive so recording is bounded.

void STDMETHODCALLTYPE Context11Proxy::IASetInputLayout(ID3D11InputLayout* pInputLayout)
{
    m_real->IASetInputLayout(pInputLayout);
    if (!m_presentHookActive) return;
    ComRefHolder layoutRef(pInputLayout);
    m_frameCommands.emplace_back(
        [this, layoutRef]()
        {
            m_real->IASetInputLayout(static_cast<ID3D11InputLayout*>(layoutRef.p));
        });
}

void STDMETHODCALLTYPE Context11Proxy::IASetVertexBuffers(
    UINT StartSlot, UINT NumBuffers, ID3D11Buffer* const* ppVertexBuffers,
    const UINT* pStrides, const UINT* pOffsets)
{
    m_real->IASetVertexBuffers(StartSlot, NumBuffers, ppVertexBuffers, pStrides, pOffsets);
    if (!m_presentHookActive) return;
    std::vector<ComRefHolder> bufRefs;
    bufRefs.reserve(NumBuffers);
    for (UINT i = 0; i < NumBuffers; ++i)
        bufRefs.emplace_back(ppVertexBuffers ? ppVertexBuffers[i] : nullptr);
    std::vector<UINT> strides;
    if (pStrides) strides.assign(pStrides, pStrides + NumBuffers);
    std::vector<UINT> offsets;
    if (pOffsets) offsets.assign(pOffsets, pOffsets + NumBuffers);
    m_frameCommands.emplace_back(
        [this, StartSlot, NumBuffers, bufRefs, strides, offsets]()
        {
            ID3D11Buffer* raw[kMaxUnwrapArray] = { 0 };
            UINT cap = NumBuffers <= kMaxUnwrapArray ? NumBuffers : kMaxUnwrapArray;
            for (UINT i = 0; i < cap; ++i)
                raw[i] = static_cast<ID3D11Buffer*>(bufRefs[i].p);
            m_real->IASetVertexBuffers(
                StartSlot, NumBuffers, raw,
                strides.empty() ? nullptr : strides.data(),
                offsets.empty() ? nullptr : offsets.data());
        });
}

void STDMETHODCALLTYPE Context11Proxy::IASetIndexBuffer(
    ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset)
{
    m_real->IASetIndexBuffer(pIndexBuffer, Format, Offset);
    if (!m_presentHookActive) return;
    ComRefHolder bufRef(pIndexBuffer);
    m_frameCommands.emplace_back(
        [this, bufRef, Format, Offset]()
        {
            m_real->IASetIndexBuffer(
                static_cast<ID3D11Buffer*>(bufRef.p), Format, Offset);
        });
}

void STDMETHODCALLTYPE Context11Proxy::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology)
{
    m_real->IASetPrimitiveTopology(Topology);
    if (!m_presentHookActive) return;
    m_frameCommands.emplace_back(
        [this, Topology]()
        {
            m_real->IASetPrimitiveTopology(Topology);
        });
}

void STDMETHODCALLTYPE Context11Proxy::RSSetState(ID3D11RasterizerState* pRasterizerState)
{
    m_real->RSSetState(pRasterizerState);
    if (!m_presentHookActive) return;
    ComRefHolder stateRef(pRasterizerState);
    m_frameCommands.emplace_back(
        [this, stateRef]()
        {
            m_real->RSSetState(static_cast<ID3D11RasterizerState*>(stateRef.p));
        });
}

void STDMETHODCALLTYPE Context11Proxy::RSSetScissorRects(UINT NumRects, const D3D11_RECT* pRects)
{
    m_real->RSSetScissorRects(NumRects, pRects);
    if (!m_presentHookActive) return;
    std::vector<D3D11_RECT> rects;
    if (pRects) rects.assign(pRects, pRects + NumRects);
    m_frameCommands.emplace_back(
        [this, NumRects, rects]()
        {
            m_real->RSSetScissorRects(
                NumRects, rects.empty() ? nullptr : rects.data());
        });
}

void STDMETHODCALLTYPE Context11Proxy::OMSetBlendState(
    ID3D11BlendState* pBlendState, const FLOAT BlendFactor[4], UINT SampleMask)
{
    m_real->OMSetBlendState(pBlendState, BlendFactor, SampleMask);
    if (!m_presentHookActive) return;
    ComRefHolder stateRef(pBlendState);
    FLOAT factor[4] = { 0, 0, 0, 0 };
    bool hasFactor = (BlendFactor != nullptr);
    if (hasFactor)
    {
        factor[0] = BlendFactor[0]; factor[1] = BlendFactor[1];
        factor[2] = BlendFactor[2]; factor[3] = BlendFactor[3];
    }
    m_frameCommands.emplace_back(
        [this, stateRef, factor, hasFactor, SampleMask]()
        {
            m_real->OMSetBlendState(
                static_cast<ID3D11BlendState*>(stateRef.p),
                hasFactor ? factor : nullptr, SampleMask);
        });
}

void STDMETHODCALLTYPE Context11Proxy::OMSetDepthStencilState(
    ID3D11DepthStencilState* pDepthStencilState, UINT StencilRef)
{
    m_real->OMSetDepthStencilState(pDepthStencilState, StencilRef);
    if (!m_presentHookActive) return;
    ComRefHolder stateRef(pDepthStencilState);
    m_frameCommands.emplace_back(
        [this, stateRef, StencilRef]()
        {
            m_real->OMSetDepthStencilState(
                static_cast<ID3D11DepthStencilState*>(stateRef.p), StencilRef);
        });
}

void STDMETHODCALLTYPE Context11Proxy::SOSetTargets(
    UINT NumBuffers, ID3D11Buffer* const* ppSOTargets, const UINT* pOffsets)
{
    m_real->SOSetTargets(NumBuffers, ppSOTargets, pOffsets);
    if (!m_presentHookActive) return;
    std::vector<ComRefHolder> bufRefs;
    bufRefs.reserve(NumBuffers);
    for (UINT i = 0; i < NumBuffers; ++i)
        bufRefs.emplace_back(ppSOTargets ? ppSOTargets[i] : nullptr);
    std::vector<UINT> offsets;
    if (pOffsets) offsets.assign(pOffsets, pOffsets + NumBuffers);
    m_frameCommands.emplace_back(
        [this, NumBuffers, bufRefs, offsets]()
        {
            ID3D11Buffer* raw[kMaxUnwrapArray] = { 0 };
            UINT cap = NumBuffers <= kMaxUnwrapArray ? NumBuffers : kMaxUnwrapArray;
            for (UINT i = 0; i < cap; ++i)
                raw[i] = static_cast<ID3D11Buffer*>(bufRefs[i].p);
            m_real->SOSetTargets(
                NumBuffers, raw,
                offsets.empty() ? nullptr : offsets.data());
        });
}

void STDMETHODCALLTYPE Context11Proxy::SetPredication(
    ID3D11Predicate* pPredicate, BOOL PredicateValue)
{
    m_real->SetPredication(pPredicate, PredicateValue);
    if (!m_presentHookActive) return;
    ComRefHolder predRef(pPredicate);
    m_frameCommands.emplace_back(
        [this, predRef, PredicateValue]()
        {
            m_real->SetPredication(
                static_cast<ID3D11Predicate*>(predRef.p), PredicateValue);
        });
}

void STDMETHODCALLTYPE Context11Proxy::CSSetUnorderedAccessViews(
    UINT StartSlot, UINT NumUAVs,
    ID3D11UnorderedAccessView* const* ppUnorderedAccessViews,
    const UINT* pUAVInitialCounts)
{
    m_real->CSSetUnorderedAccessViews(StartSlot, NumUAVs, ppUnorderedAccessViews, pUAVInitialCounts);
    if (!m_presentHookActive) return;
    std::vector<ComRefHolder> uavRefs;
    uavRefs.reserve(NumUAVs);
    for (UINT i = 0; i < NumUAVs; ++i)
        uavRefs.emplace_back(ppUnorderedAccessViews ? ppUnorderedAccessViews[i] : nullptr);
    std::vector<UINT> initialCounts;
    if (pUAVInitialCounts)
        initialCounts.assign(pUAVInitialCounts, pUAVInitialCounts + NumUAVs);
    m_frameCommands.emplace_back(
        [this, StartSlot, NumUAVs, uavRefs, initialCounts]()
        {
            ID3D11UnorderedAccessView* raw[kMaxUnwrapArray] = { 0 };
            UINT cap = NumUAVs <= kMaxUnwrapArray ? NumUAVs : kMaxUnwrapArray;
            for (UINT i = 0; i < cap; ++i)
                raw[i] = static_cast<ID3D11UnorderedAccessView*>(uavRefs[i].p);
            m_real->CSSetUnorderedAccessViews(
                StartSlot, NumUAVs, raw,
                initialCounts.empty() ? nullptr : initialCounts.data());
        });
}

void STDMETHODCALLTYPE Context11Proxy::GetDevice(ID3D11Device** ppDevice)
{
    // COM identity: GetDevice must return the wrapped device, not the real
    // one — otherwise a game that round-trips through GetDevice ends up
    // bypassing our wrapper for subsequent resource creation.
    if (!ppDevice) return;
    if (m_parent)
    {
        // Device11Proxy publicly inherits from ID3D11Device — real upcast,
        // static_cast keeps /W4 + warnings-as-errors happy.
        *ppDevice = static_cast<ID3D11Device*>(m_parent);
        m_parent->AddRef();
        return;
    }
    m_real->GetDevice(ppDevice);
}

} // namespace wiz3d
