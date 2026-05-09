/* NvDirectMode - IDirect3DDevice9 proxy (passthrough)
 *
 * Stage 1b-i implementation. Every method forwards verbatim. The point of
 * this layer is to *exist* — once we know the wrapping doesn't break
 * anything, 1b-iii hooks GetBackBuffer / CreateRenderTarget for buffer
 * doubling and 1b-iv hooks SetRenderTarget for per-eye routing.
 */

#include "Device9Proxy.h"
#include "swapchain_helpers.h"
#include "eye_state.h"
#include "log.h"

namespace NvDirectMode
{

// Primary-device registry for the eye-change callback dispatcher.
// Mirror of the d3d11/d3d10 SwapChainProxy primary-pointer pattern.
namespace
{
    Device9Proxy*    g_primaryDevice = nullptr;
    CRITICAL_SECTION g_primaryLock;
    bool             g_primaryLockInit = false;

    void EnsurePrimaryLock()
    {
        if (g_primaryLockInit) return;
        InitializeCriticalSection(&g_primaryLock);
        g_primaryLockInit = true;
    }

    void OnEyeChange(int oldEye, int /*newEye*/)
    {
        EnsurePrimaryLock();
        EnterCriticalSection(&g_primaryLock);
        if (g_primaryDevice) g_primaryDevice->CaptureEye(oldEye);
        LeaveCriticalSection(&g_primaryLock);
    }
}

Device9Proxy::Device9Proxy(IDirect3DDevice9* real, bool isEx)
    : m_real(real)
    , m_realEx(isEx ? static_cast<IDirect3DDevice9Ex*>(real) : nullptr)
    , m_isEx(isEx)
    , m_refs(1)
    , m_logicalWidth(0)
    , m_logicalHeight(0)
    , m_pTrackedBackBuffer(nullptr)
    , m_shadowBB(nullptr)
    , m_leftEyeSurf(nullptr)
    , m_rightEyeSurf(nullptr)
{
    EnsurePrimaryLock();
    EnterCriticalSection(&g_primaryLock);
    g_primaryDevice = this;
    LeaveCriticalSection(&g_primaryLock);
    NvDirectMode::RegisterEyeChangeHandler(&OnEyeChange);
}

Device9Proxy::~Device9Proxy()
{
    EnsurePrimaryLock();
    EnterCriticalSection(&g_primaryLock);
    if (g_primaryDevice == this) g_primaryDevice = nullptr;
    LeaveCriticalSection(&g_primaryLock);

    ReleaseShadow();
    ReleaseBackBufferReference();
}

void Device9Proxy::ReleaseShadow()
{
    if (m_shadowBB)     { m_shadowBB->Release();     m_shadowBB = nullptr; }
    if (m_leftEyeSurf)  { m_leftEyeSurf->Release();  m_leftEyeSurf = nullptr; }
    if (m_rightEyeSurf) { m_rightEyeSurf->Release(); m_rightEyeSurf = nullptr; }
}

void Device9Proxy::EnsureShadow()
{
    if (m_shadowBB || !m_real || !m_pTrackedBackBuffer) return;

    D3DSURFACE_DESC desc = {};
    if (FAILED(m_pTrackedBackBuffer->GetDesc(&desc))) return;
    m_logicalWidth = desc.Width;
    m_logicalHeight = desc.Height;

    // Shadow has same format/MS as the real BB so StretchRect can copy
    // between them without conversion.
    HRESULT hr = m_real->CreateRenderTarget(desc.Width, desc.Height, desc.Format,
                                             desc.MultiSampleType, desc.MultiSampleQuality,
                                             FALSE, &m_shadowBB, NULL);
    if (FAILED(hr) || !m_shadowBB)
    {
        LOG_VERBOSE("  d3d9 EnsureShadow: CreateRenderTarget(%ux%u) FAILED hr=0x%08lX\n",
                    desc.Width, desc.Height, hr);
        m_shadowBB = nullptr;
        return;
    }
    LOG_VERBOSE("  d3d9 EnsureShadow: shadow=%p (%ux%u, fmt=%d) for realBB=%p\n",
                m_shadowBB, desc.Width, desc.Height, (int)desc.Format,
                (void*)m_pTrackedBackBuffer);
}

void Device9Proxy::CaptureEye(int eyeBeingLeft)
{
    if (!m_shadowBB || !m_real) return;
    IDirect3DSurface9** slot = nullptr;
    if      (eyeBeingLeft == NvDirectMode::kEyeLeft)  slot = &m_leftEyeSurf;
    else if (eyeBeingLeft == NvDirectMode::kEyeRight) slot = &m_rightEyeSurf;
    else return;

    if (!*slot)
    {
        D3DSURFACE_DESC desc = {};
        m_shadowBB->GetDesc(&desc);
        HRESULT hr = m_real->CreateRenderTarget(desc.Width, desc.Height, desc.Format,
                                                 desc.MultiSampleType, desc.MultiSampleQuality,
                                                 FALSE, slot, NULL);
        if (FAILED(hr) || !*slot) return;
        LOG_VERBOSE("  d3d9 CaptureEye(%d): allocated eye surface=%p\n", eyeBeingLeft, *slot);
    }

    m_real->StretchRect(m_shadowBB, NULL, *slot, NULL, D3DTEXF_NONE);
    NVDM_TRACE_FIRST_N(8, "  d3d9 CaptureEye(eye=%d): StretchRect shadow=%p -> eyeSurf=%p\n",
                       eyeBeingLeft, m_shadowBB, *slot);
}

void Device9Proxy::CompositeAndPresent()
{
    if (!m_real || !m_pTrackedBackBuffer) return;

    int currentEye = NvDirectMode::GetActiveEye();
    if (currentEye == NvDirectMode::kEyeLeft || currentEye == NvDirectMode::kEyeRight)
        CaptureEye(currentEye);

    bool topBottom = NvDM_OutputIsTopBottom() != 0;
    bool swap      = NvDM_SwapEyes() != 0;
    IDirect3DSurface9* leftSrc  = swap ? m_rightEyeSurf : m_leftEyeSurf;
    IDirect3DSurface9* rightSrc = swap ? m_leftEyeSurf  : m_rightEyeSurf;

    D3DSURFACE_DESC bbDesc = {};
    m_pTrackedBackBuffer->GetDesc(&bbDesc);

    if (leftSrc && rightSrc)
    {
        // Composite: each eye into half the real BB.
        RECT leftRect, rightRect;
        if (topBottom)
        {
            leftRect  = { 0, 0, (LONG)bbDesc.Width, (LONG)(bbDesc.Height / 2) };
            rightRect = { 0, (LONG)(bbDesc.Height / 2), (LONG)bbDesc.Width, (LONG)bbDesc.Height };
        }
        else
        {
            leftRect  = { 0, 0, (LONG)(bbDesc.Width / 2), (LONG)bbDesc.Height };
            rightRect = { (LONG)(bbDesc.Width / 2), 0, (LONG)bbDesc.Width, (LONG)bbDesc.Height };
        }
        m_real->StretchRect(leftSrc,  NULL, m_pTrackedBackBuffer, &leftRect,  D3DTEXF_LINEAR);
        m_real->StretchRect(rightSrc, NULL, m_pTrackedBackBuffer, &rightRect, D3DTEXF_LINEAR);
        NVDM_TRACE_FIRST_N(2, "  d3d9 CompositeAndPresent: %s eyes (swap=%d) -> realBB=%p\n",
                           topBottom ? "T-B" : "SBS", (int)swap, (void*)m_pTrackedBackBuffer);
    }
    else
    {
        // Single eye / mono fallback.
        IDirect3DSurface9* src = leftSrc ? leftSrc : (rightSrc ? rightSrc : m_shadowBB);
        if (src)
            m_real->StretchRect(src, NULL, m_pTrackedBackBuffer, NULL, D3DTEXF_NONE);
        NVDM_TRACE_FIRST_N(4, "  d3d9 CompositeAndPresent (fallback): src=%s\n",
                           (src == m_leftEyeSurf  ? "leftEye"  :
                            src == m_rightEyeSurf ? "rightEye" :
                            src == m_shadowBB     ? "shadow"   : "none"));
    }
}

void Device9Proxy::SetLogicalBackBufferSize(UINT w, UINT h)
{
    m_logicalWidth  = w;
    m_logicalHeight = h;
}

void Device9Proxy::StashBackBufferReference()
{
    ReleaseBackBufferReference();
    ReleaseShadow();
    if (!m_real) return;
    // GetBackBuffer adds a ref; we hold that ref until Reset/destroy.
    m_real->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pTrackedBackBuffer);
    EnsureShadow();
}

void Device9Proxy::ReleaseBackBufferReference()
{
    if (m_pTrackedBackBuffer)
    {
        m_pTrackedBackBuffer->Release();
        m_pTrackedBackBuffer = nullptr;
    }
}

HRESULT STDMETHODCALLTYPE Device9Proxy::QueryInterface(REFIID riid, void** ppvObj)
{
    if (!ppvObj) return E_POINTER;
    if (riid == IID_IUnknown || riid == IID_IDirect3DDevice9)
    {
        *ppvObj = static_cast<IDirect3DDevice9*>(this);
        AddRef();
        return S_OK;
    }
    if (riid == IID_IDirect3DDevice9Ex)
    {
        if (!m_isEx) { *ppvObj = nullptr; return E_NOINTERFACE; }
        *ppvObj = static_cast<IDirect3DDevice9Ex*>(this);
        AddRef();
        return S_OK;
    }
    HRESULT hr = m_real->QueryInterface(riid, ppvObj);
    NVDM_TRACE_FIRST_N(8, "  Device9Proxy::QI(unknown IID) hr=0x%08lX -- bypass risk\n", hr);
    return hr;
}

ULONG STDMETHODCALLTYPE Device9Proxy::AddRef()  { return InterlockedIncrement(&m_refs); }
ULONG STDMETHODCALLTYPE Device9Proxy::Release()
{
    LONG r = InterlockedDecrement(&m_refs);
    if (r == 0)
    {
        if (m_real) { m_real->Release(); m_real = nullptr; }
        delete this;
    }
    return (ULONG)r;
}

// ---------------------------------------------------------------------------
// IDirect3DDevice9 — pure forwarders
// ---------------------------------------------------------------------------
HRESULT Device9Proxy::TestCooperativeLevel()                                          { return m_real->TestCooperativeLevel(); }
UINT    Device9Proxy::GetAvailableTextureMem()                                        { return m_real->GetAvailableTextureMem(); }
HRESULT Device9Proxy::EvictManagedResources()                                         { return m_real->EvictManagedResources(); }
HRESULT Device9Proxy::GetDirect3D(IDirect3D9** ppD3D9)                                { return m_real->GetDirect3D(ppD3D9); }
HRESULT Device9Proxy::GetDeviceCaps(D3DCAPS9* pCaps)                                  { return m_real->GetDeviceCaps(pCaps); }
HRESULT Device9Proxy::GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode)          { return m_real->GetDisplayMode(iSwapChain, pMode); }
HRESULT Device9Proxy::GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS* p)         { return m_real->GetCreationParameters(p); }
HRESULT Device9Proxy::SetCursorProperties(UINT X, UINT Y, IDirect3DSurface9* pBmp)    { return m_real->SetCursorProperties(X, Y, pBmp); }
void    Device9Proxy::SetCursorPosition(int X, int Y, DWORD Flags)                    { m_real->SetCursorPosition(X, Y, Flags); }
BOOL    Device9Proxy::ShowCursor(BOOL bShow)                                          { return m_real->ShowCursor(bShow); }
HRESULT Device9Proxy::CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* p, IDirect3DSwapChain9** pp) { return m_real->CreateAdditionalSwapChain(p, pp); }
HRESULT Device9Proxy::GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pp)         { return m_real->GetSwapChain(iSwapChain, pp); }
UINT    Device9Proxy::GetNumberOfSwapChains()                                         { return m_real->GetNumberOfSwapChains(); }
HRESULT Device9Proxy::Reset(D3DPRESENT_PARAMETERS* p)
{
    // Old back buffer dies in Reset; release our tracking ref before forwarding.
    ReleaseBackBufferReference();

    D3DPRESENT_PARAMETERS modified;
    UINT logicalW = 0, logicalH = 0;
    if (p)
    {
        modified = *p;
        ResolveAndDoubleSwapchainParams(&modified, modified.hDeviceWindow, &logicalW, &logicalH);
        p = &modified;
    }
    HRESULT hr = m_real->Reset(p);
    if (SUCCEEDED(hr))
    {
        if (logicalW > 0) SetLogicalBackBufferSize(logicalW, logicalH);
        StashBackBufferReference();
    }
    return hr;
}

HRESULT Device9Proxy::Present(CONST RECT* sr, CONST RECT* dr, HWND h, CONST RGNDATA* d)
{
    // Stage 4: composite captured eyes into the real BB before forwarding,
    // so the Present writes a SBS / T-B / mono image to the visible surface.
    CompositeAndPresent();
    return m_real->Present(sr, dr, h, d);
}
HRESULT Device9Proxy::GetBackBuffer(UINT iSC, UINT iBB, D3DBACKBUFFER_TYPE T, IDirect3DSurface9** pp)
{
    // Hand the game our shadow surface (logical size, format-matched to
    // the real BB) instead of the real BB itself. Game's draws land in
    // the shadow; CompositeAndPresent resolves them into the real BB at
    // Present time.
    if (iSC == 0 && iBB == 0 && T == D3DBACKBUFFER_TYPE_MONO && pp)
    {
        EnsureShadow();
        if (m_shadowBB)
        {
            m_shadowBB->AddRef();
            *pp = m_shadowBB;
            NVDM_TRACE_FIRST_N(4, "  d3d9 GetBackBuffer(0): handed shadow %p\n", m_shadowBB);
            return S_OK;
        }
    }
    return m_real->GetBackBuffer(iSC, iBB, T, pp);
}
HRESULT Device9Proxy::GetRasterStatus(UINT iSC, D3DRASTER_STATUS* p)                  { return m_real->GetRasterStatus(iSC, p); }
HRESULT Device9Proxy::SetDialogBoxMode(BOOL bEnable)                                  { return m_real->SetDialogBoxMode(bEnable); }
void    Device9Proxy::SetGammaRamp(UINT iSC, DWORD F, CONST D3DGAMMARAMP* p)          { m_real->SetGammaRamp(iSC, F, p); }
void    Device9Proxy::GetGammaRamp(UINT iSC, D3DGAMMARAMP* p)                         { m_real->GetGammaRamp(iSC, p); }
HRESULT Device9Proxy::CreateTexture(UINT W, UINT H, UINT L, DWORD U, D3DFORMAT F, D3DPOOL P, IDirect3DTexture9** pp, HANDLE* sh)            { return m_real->CreateTexture(W, H, L, U, F, P, pp, sh); }
HRESULT Device9Proxy::CreateVolumeTexture(UINT W, UINT H, UINT D, UINT L, DWORD U, D3DFORMAT F, D3DPOOL P, IDirect3DVolumeTexture9** pp, HANDLE* sh) { return m_real->CreateVolumeTexture(W, H, D, L, U, F, P, pp, sh); }
HRESULT Device9Proxy::CreateCubeTexture(UINT E, UINT L, DWORD U, D3DFORMAT F, D3DPOOL P, IDirect3DCubeTexture9** pp, HANDLE* sh)             { return m_real->CreateCubeTexture(E, L, U, F, P, pp, sh); }
HRESULT Device9Proxy::CreateVertexBuffer(UINT L, DWORD U, DWORD F, D3DPOOL P, IDirect3DVertexBuffer9** pp, HANDLE* sh)                       { return m_real->CreateVertexBuffer(L, U, F, P, pp, sh); }
HRESULT Device9Proxy::CreateIndexBuffer(UINT L, DWORD U, D3DFORMAT F, D3DPOOL P, IDirect3DIndexBuffer9** pp, HANDLE* sh)                     { return m_real->CreateIndexBuffer(L, U, F, P, pp, sh); }
HRESULT Device9Proxy::CreateRenderTarget(UINT W, UINT H, D3DFORMAT F, D3DMULTISAMPLE_TYPE M, DWORD MQ, BOOL Lk, IDirect3DSurface9** pp, HANDLE* sh) { return m_real->CreateRenderTarget(W, H, F, M, MQ, Lk, pp, sh); }
HRESULT Device9Proxy::CreateDepthStencilSurface(UINT W, UINT H, D3DFORMAT F, D3DMULTISAMPLE_TYPE M, DWORD MQ, BOOL Dc, IDirect3DSurface9** pp, HANDLE* sh) { return m_real->CreateDepthStencilSurface(W, H, F, M, MQ, Dc, pp, sh); }
HRESULT Device9Proxy::UpdateSurface(IDirect3DSurface9* sS, CONST RECT* sR, IDirect3DSurface9* dS, CONST POINT* dP) { return m_real->UpdateSurface(sS, sR, dS, dP); }
HRESULT Device9Proxy::UpdateTexture(IDirect3DBaseTexture9* sT, IDirect3DBaseTexture9* dT)                          { return m_real->UpdateTexture(sT, dT); }
HRESULT Device9Proxy::GetRenderTargetData(IDirect3DSurface9* sR, IDirect3DSurface9* dS)                            { return m_real->GetRenderTargetData(sR, dS); }
HRESULT Device9Proxy::GetFrontBufferData(UINT iSC, IDirect3DSurface9* dS)                                          { return m_real->GetFrontBufferData(iSC, dS); }
HRESULT Device9Proxy::StretchRect(IDirect3DSurface9* sS, CONST RECT* sR, IDirect3DSurface9* dS, CONST RECT* dR, D3DTEXTUREFILTERTYPE F) { return m_real->StretchRect(sS, sR, dS, dR, F); }
HRESULT Device9Proxy::ColorFill(IDirect3DSurface9* s, CONST RECT* r, D3DCOLOR c)                                   { return m_real->ColorFill(s, r, c); }
HRESULT Device9Proxy::CreateOffscreenPlainSurface(UINT W, UINT H, D3DFORMAT F, D3DPOOL P, IDirect3DSurface9** pp, HANDLE* sh)              { return m_real->CreateOffscreenPlainSurface(W, H, F, P, pp, sh); }
HRESULT Device9Proxy::SetRenderTarget(DWORD i, IDirect3DSurface9* p)
{
    HRESULT hr = m_real->SetRenderTarget(i, p);
    // Stage 3 v2 / shadow-RT: shadow is at logical (1x) size, so no
    // per-eye viewport clamp needed any more — game's natural rendering
    // goes straight into the shadow at the size the game expects.
    // BB-bound logging stays for diagnostics.
    if (i == 0 && p == m_shadowBB)
        NVDM_TRACE_FIRST_N(8, "  d3d9 SetRenderTarget BB(shadow=%p) bound (no clamp \xe2\x80\x94 shadow is 1x)\n",
                           m_shadowBB);
    return hr;
}
HRESULT Device9Proxy::GetRenderTarget(DWORD i, IDirect3DSurface9** pp)                                             { return m_real->GetRenderTarget(i, pp); }
HRESULT Device9Proxy::SetDepthStencilSurface(IDirect3DSurface9* p)                                                 { return m_real->SetDepthStencilSurface(p); }
HRESULT Device9Proxy::GetDepthStencilSurface(IDirect3DSurface9** pp)                                               { return m_real->GetDepthStencilSurface(pp); }
HRESULT Device9Proxy::BeginScene()                                                                                 { return m_real->BeginScene(); }
HRESULT Device9Proxy::EndScene()                                                                                   { return m_real->EndScene(); }
HRESULT Device9Proxy::Clear(DWORD C, CONST D3DRECT* pR, DWORD F, D3DCOLOR Co, float Z, DWORD S)                    { return m_real->Clear(C, pR, F, Co, Z, S); }
HRESULT Device9Proxy::SetTransform(D3DTRANSFORMSTATETYPE S, CONST D3DMATRIX* M)                                    { return m_real->SetTransform(S, M); }
HRESULT Device9Proxy::GetTransform(D3DTRANSFORMSTATETYPE S, D3DMATRIX* M)                                          { return m_real->GetTransform(S, M); }
HRESULT Device9Proxy::MultiplyTransform(D3DTRANSFORMSTATETYPE S, CONST D3DMATRIX* M)                               { return m_real->MultiplyTransform(S, M); }
HRESULT Device9Proxy::SetViewport(CONST D3DVIEWPORT9* p)                                                           { return m_real->SetViewport(p); }
HRESULT Device9Proxy::GetViewport(D3DVIEWPORT9* p)                                                                 { return m_real->GetViewport(p); }
HRESULT Device9Proxy::SetMaterial(CONST D3DMATERIAL9* p)                                                           { return m_real->SetMaterial(p); }
HRESULT Device9Proxy::GetMaterial(D3DMATERIAL9* p)                                                                 { return m_real->GetMaterial(p); }
HRESULT Device9Proxy::SetLight(DWORD i, CONST D3DLIGHT9* p)                                                        { return m_real->SetLight(i, p); }
HRESULT Device9Proxy::GetLight(DWORD i, D3DLIGHT9* p)                                                              { return m_real->GetLight(i, p); }
HRESULT Device9Proxy::LightEnable(DWORD i, BOOL b)                                                                 { return m_real->LightEnable(i, b); }
HRESULT Device9Proxy::GetLightEnable(DWORD i, BOOL* p)                                                             { return m_real->GetLightEnable(i, p); }
HRESULT Device9Proxy::SetClipPlane(DWORD i, CONST float* p)                                                        { return m_real->SetClipPlane(i, p); }
HRESULT Device9Proxy::GetClipPlane(DWORD i, float* p)                                                              { return m_real->GetClipPlane(i, p); }
HRESULT Device9Proxy::SetRenderState(D3DRENDERSTATETYPE S, DWORD V)                                                { return m_real->SetRenderState(S, V); }
HRESULT Device9Proxy::GetRenderState(D3DRENDERSTATETYPE S, DWORD* p)                                               { return m_real->GetRenderState(S, p); }
HRESULT Device9Proxy::CreateStateBlock(D3DSTATEBLOCKTYPE T, IDirect3DStateBlock9** pp)                             { return m_real->CreateStateBlock(T, pp); }
HRESULT Device9Proxy::BeginStateBlock()                                                                            { return m_real->BeginStateBlock(); }
HRESULT Device9Proxy::EndStateBlock(IDirect3DStateBlock9** pp)                                                     { return m_real->EndStateBlock(pp); }
HRESULT Device9Proxy::SetClipStatus(CONST D3DCLIPSTATUS9* p)                                                       { return m_real->SetClipStatus(p); }
HRESULT Device9Proxy::GetClipStatus(D3DCLIPSTATUS9* p)                                                             { return m_real->GetClipStatus(p); }
HRESULT Device9Proxy::GetTexture(DWORD S, IDirect3DBaseTexture9** pp)                                              { return m_real->GetTexture(S, pp); }
HRESULT Device9Proxy::SetTexture(DWORD S, IDirect3DBaseTexture9* p)                                                { return m_real->SetTexture(S, p); }
HRESULT Device9Proxy::GetTextureStageState(DWORD S, D3DTEXTURESTAGESTATETYPE T, DWORD* p)                          { return m_real->GetTextureStageState(S, T, p); }
HRESULT Device9Proxy::SetTextureStageState(DWORD S, D3DTEXTURESTAGESTATETYPE T, DWORD V)                           { return m_real->SetTextureStageState(S, T, V); }
HRESULT Device9Proxy::GetSamplerState(DWORD S, D3DSAMPLERSTATETYPE T, DWORD* p)                                    { return m_real->GetSamplerState(S, T, p); }
HRESULT Device9Proxy::SetSamplerState(DWORD S, D3DSAMPLERSTATETYPE T, DWORD V)                                     { return m_real->SetSamplerState(S, T, V); }
HRESULT Device9Proxy::ValidateDevice(DWORD* p)                                                                     { return m_real->ValidateDevice(p); }
HRESULT Device9Proxy::SetPaletteEntries(UINT N, CONST PALETTEENTRY* p)                                             { return m_real->SetPaletteEntries(N, p); }
HRESULT Device9Proxy::GetPaletteEntries(UINT N, PALETTEENTRY* p)                                                   { return m_real->GetPaletteEntries(N, p); }
HRESULT Device9Proxy::SetCurrentTexturePalette(UINT N)                                                             { return m_real->SetCurrentTexturePalette(N); }
HRESULT Device9Proxy::GetCurrentTexturePalette(UINT* p)                                                            { return m_real->GetCurrentTexturePalette(p); }
HRESULT Device9Proxy::SetScissorRect(CONST RECT* p)                                                                { return m_real->SetScissorRect(p); }
HRESULT Device9Proxy::GetScissorRect(RECT* p)                                                                      { return m_real->GetScissorRect(p); }
HRESULT Device9Proxy::SetSoftwareVertexProcessing(BOOL b)                                                          { return m_real->SetSoftwareVertexProcessing(b); }
BOOL    Device9Proxy::GetSoftwareVertexProcessing()                                                                { return m_real->GetSoftwareVertexProcessing(); }
HRESULT Device9Proxy::SetNPatchMode(float n)                                                                       { return m_real->SetNPatchMode(n); }
float   Device9Proxy::GetNPatchMode()                                                                              { return m_real->GetNPatchMode(); }
HRESULT Device9Proxy::DrawPrimitive(D3DPRIMITIVETYPE T, UINT SV, UINT PC)                                          { return m_real->DrawPrimitive(T, SV, PC); }
HRESULT Device9Proxy::DrawIndexedPrimitive(D3DPRIMITIVETYPE T, INT BV, UINT MV, UINT NV, UINT SI, UINT PC)         { return m_real->DrawIndexedPrimitive(T, BV, MV, NV, SI, PC); }
HRESULT Device9Proxy::DrawPrimitiveUP(D3DPRIMITIVETYPE T, UINT PC, CONST void* p, UINT St)                         { return m_real->DrawPrimitiveUP(T, PC, p, St); }
HRESULT Device9Proxy::DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE T, UINT MV, UINT NV, UINT PC, CONST void* iD, D3DFORMAT iF, CONST void* vD, UINT vS) { return m_real->DrawIndexedPrimitiveUP(T, MV, NV, PC, iD, iF, vD, vS); }
HRESULT Device9Proxy::ProcessVertices(UINT SS, UINT DI, UINT VC, IDirect3DVertexBuffer9* pDB, IDirect3DVertexDeclaration9* pVD, DWORD F)   { return m_real->ProcessVertices(SS, DI, VC, pDB, pVD, F); }
HRESULT Device9Proxy::CreateVertexDeclaration(CONST D3DVERTEXELEMENT9* pE, IDirect3DVertexDeclaration9** pp)       { return m_real->CreateVertexDeclaration(pE, pp); }
HRESULT Device9Proxy::SetVertexDeclaration(IDirect3DVertexDeclaration9* p)                                         { return m_real->SetVertexDeclaration(p); }
HRESULT Device9Proxy::GetVertexDeclaration(IDirect3DVertexDeclaration9** pp)                                       { return m_real->GetVertexDeclaration(pp); }
HRESULT Device9Proxy::SetFVF(DWORD F)                                                                              { return m_real->SetFVF(F); }
HRESULT Device9Proxy::GetFVF(DWORD* p)                                                                             { return m_real->GetFVF(p); }
HRESULT Device9Proxy::CreateVertexShader(CONST DWORD* p, IDirect3DVertexShader9** pp)                              { return m_real->CreateVertexShader(p, pp); }
HRESULT Device9Proxy::SetVertexShader(IDirect3DVertexShader9* p)                                                   { return m_real->SetVertexShader(p); }
HRESULT Device9Proxy::GetVertexShader(IDirect3DVertexShader9** pp)                                                 { return m_real->GetVertexShader(pp); }
HRESULT Device9Proxy::SetVertexShaderConstantF(UINT SR, CONST float* p, UINT C)                                    { return m_real->SetVertexShaderConstantF(SR, p, C); }
HRESULT Device9Proxy::GetVertexShaderConstantF(UINT SR, float* p, UINT C)                                          { return m_real->GetVertexShaderConstantF(SR, p, C); }
HRESULT Device9Proxy::SetVertexShaderConstantI(UINT SR, CONST int* p, UINT C)                                      { return m_real->SetVertexShaderConstantI(SR, p, C); }
HRESULT Device9Proxy::GetVertexShaderConstantI(UINT SR, int* p, UINT C)                                            { return m_real->GetVertexShaderConstantI(SR, p, C); }
HRESULT Device9Proxy::SetVertexShaderConstantB(UINT SR, CONST BOOL* p, UINT C)                                     { return m_real->SetVertexShaderConstantB(SR, p, C); }
HRESULT Device9Proxy::GetVertexShaderConstantB(UINT SR, BOOL* p, UINT C)                                           { return m_real->GetVertexShaderConstantB(SR, p, C); }
HRESULT Device9Proxy::SetStreamSource(UINT SN, IDirect3DVertexBuffer9* pSD, UINT OB, UINT St)                      { return m_real->SetStreamSource(SN, pSD, OB, St); }
HRESULT Device9Proxy::GetStreamSource(UINT SN, IDirect3DVertexBuffer9** pp, UINT* pO, UINT* pS)                    { return m_real->GetStreamSource(SN, pp, pO, pS); }
HRESULT Device9Proxy::SetStreamSourceFreq(UINT SN, UINT S)                                                         { return m_real->SetStreamSourceFreq(SN, S); }
HRESULT Device9Proxy::GetStreamSourceFreq(UINT SN, UINT* p)                                                        { return m_real->GetStreamSourceFreq(SN, p); }
HRESULT Device9Proxy::SetIndices(IDirect3DIndexBuffer9* p)                                                         { return m_real->SetIndices(p); }
HRESULT Device9Proxy::GetIndices(IDirect3DIndexBuffer9** pp)                                                       { return m_real->GetIndices(pp); }
HRESULT Device9Proxy::CreatePixelShader(CONST DWORD* p, IDirect3DPixelShader9** pp)                                { return m_real->CreatePixelShader(p, pp); }
HRESULT Device9Proxy::SetPixelShader(IDirect3DPixelShader9* p)                                                     { return m_real->SetPixelShader(p); }
HRESULT Device9Proxy::GetPixelShader(IDirect3DPixelShader9** pp)                                                   { return m_real->GetPixelShader(pp); }
HRESULT Device9Proxy::SetPixelShaderConstantF(UINT SR, CONST float* p, UINT C)                                     { return m_real->SetPixelShaderConstantF(SR, p, C); }
HRESULT Device9Proxy::GetPixelShaderConstantF(UINT SR, float* p, UINT C)                                           { return m_real->GetPixelShaderConstantF(SR, p, C); }
HRESULT Device9Proxy::SetPixelShaderConstantI(UINT SR, CONST int* p, UINT C)                                       { return m_real->SetPixelShaderConstantI(SR, p, C); }
HRESULT Device9Proxy::GetPixelShaderConstantI(UINT SR, int* p, UINT C)                                             { return m_real->GetPixelShaderConstantI(SR, p, C); }
HRESULT Device9Proxy::SetPixelShaderConstantB(UINT SR, CONST BOOL* p, UINT C)                                      { return m_real->SetPixelShaderConstantB(SR, p, C); }
HRESULT Device9Proxy::GetPixelShaderConstantB(UINT SR, BOOL* p, UINT C)                                            { return m_real->GetPixelShaderConstantB(SR, p, C); }
HRESULT Device9Proxy::DrawRectPatch(UINT H, CONST float* p, CONST D3DRECTPATCH_INFO* pI)                           { return m_real->DrawRectPatch(H, p, pI); }
HRESULT Device9Proxy::DrawTriPatch(UINT H, CONST float* p, CONST D3DTRIPATCH_INFO* pI)                             { return m_real->DrawTriPatch(H, p, pI); }
HRESULT Device9Proxy::DeletePatch(UINT H)                                                                          { return m_real->DeletePatch(H); }
HRESULT Device9Proxy::CreateQuery(D3DQUERYTYPE T, IDirect3DQuery9** pp)                                            { return m_real->CreateQuery(T, pp); }

// ---------------------------------------------------------------------------
// IDirect3DDevice9Ex extras — only reachable when m_isEx (QI gates the cast)
// ---------------------------------------------------------------------------
HRESULT Device9Proxy::SetConvolutionMonoKernel(UINT W, UINT H, float* r, float* c)                                                                  { return m_realEx->SetConvolutionMonoKernel(W, H, r, c); }
HRESULT Device9Proxy::ComposeRects(IDirect3DSurface9* pS, IDirect3DSurface9* pD, IDirect3DVertexBuffer9* pSR, UINT N, IDirect3DVertexBuffer9* pDR, D3DCOMPOSERECTSOP O, INT X, INT Y) { return m_realEx->ComposeRects(pS, pD, pSR, N, pDR, O, X, Y); }
HRESULT Device9Proxy::PresentEx(CONST RECT* sR, CONST RECT* dR, HWND h, CONST RGNDATA* pD, DWORD F)
{
    // Same composite as Present() — capture latest eye + StretchRect both
    // eyes (or fallback) into the real BB, then forward.
    CompositeAndPresent();
    return m_realEx->PresentEx(sR, dR, h, pD, F);
}
HRESULT Device9Proxy::GetGPUThreadPriority(INT* p)                                                                                                  { return m_realEx->GetGPUThreadPriority(p); }
HRESULT Device9Proxy::SetGPUThreadPriority(INT P)                                                                                                   { return m_realEx->SetGPUThreadPriority(P); }
HRESULT Device9Proxy::WaitForVBlank(UINT iSC)                                                                                                       { return m_realEx->WaitForVBlank(iSC); }
HRESULT Device9Proxy::CheckResourceResidency(IDirect3DResource9** pRA, UINT32 N)                                                                    { return m_realEx->CheckResourceResidency(pRA, N); }
HRESULT Device9Proxy::SetMaximumFrameLatency(UINT M)                                                                                                { return m_realEx->SetMaximumFrameLatency(M); }
HRESULT Device9Proxy::GetMaximumFrameLatency(UINT* p)                                                                                               { return m_realEx->GetMaximumFrameLatency(p); }
HRESULT Device9Proxy::CheckDeviceState(HWND h)                                                                                                      { return m_realEx->CheckDeviceState(h); }
HRESULT Device9Proxy::CreateRenderTargetEx(UINT W, UINT H, D3DFORMAT F, D3DMULTISAMPLE_TYPE M, DWORD MQ, BOOL L, IDirect3DSurface9** pp, HANDLE* sh, DWORD U)            { return m_realEx->CreateRenderTargetEx(W, H, F, M, MQ, L, pp, sh, U); }
HRESULT Device9Proxy::CreateOffscreenPlainSurfaceEx(UINT W, UINT H, D3DFORMAT F, D3DPOOL P, IDirect3DSurface9** pp, HANDLE* sh, DWORD U)                                 { return m_realEx->CreateOffscreenPlainSurfaceEx(W, H, F, P, pp, sh, U); }
HRESULT Device9Proxy::CreateDepthStencilSurfaceEx(UINT W, UINT H, D3DFORMAT F, D3DMULTISAMPLE_TYPE M, DWORD MQ, BOOL D, IDirect3DSurface9** pp, HANDLE* sh, DWORD U)     { return m_realEx->CreateDepthStencilSurfaceEx(W, H, F, M, MQ, D, pp, sh, U); }
HRESULT Device9Proxy::ResetEx(D3DPRESENT_PARAMETERS* pP, D3DDISPLAYMODEEX* pF)
{
    ReleaseBackBufferReference();

    D3DPRESENT_PARAMETERS modified;
    UINT logicalW = 0, logicalH = 0;
    if (pP)
    {
        modified = *pP;
        ResolveAndDoubleSwapchainParams(&modified, modified.hDeviceWindow, &logicalW, &logicalH);
        pP = &modified;
    }
    HRESULT hr = m_realEx->ResetEx(pP, pF);
    if (SUCCEEDED(hr))
    {
        if (logicalW > 0) SetLogicalBackBufferSize(logicalW, logicalH);
        StashBackBufferReference();
    }
    return hr;
}
HRESULT Device9Proxy::GetDisplayModeEx(UINT iSC, D3DDISPLAYMODEEX* p, D3DDISPLAYROTATION* pR)                                                       { return m_realEx->GetDisplayModeEx(iSC, p, pR); }

} // namespace NvDirectMode
