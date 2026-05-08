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

namespace NvDirectMode
{

Device9Proxy::Device9Proxy(IDirect3DDevice9* real, bool isEx)
    : m_real(real)
    , m_realEx(isEx ? static_cast<IDirect3DDevice9Ex*>(real) : nullptr)
    , m_isEx(isEx)
    , m_refs(1)
    , m_logicalWidth(0)
    , m_logicalHeight(0)
    , m_pTrackedBackBuffer(nullptr)
{
}

Device9Proxy::~Device9Proxy()
{
    ReleaseBackBufferReference();
}

void Device9Proxy::SetLogicalBackBufferSize(UINT w, UINT h)
{
    m_logicalWidth  = w;
    m_logicalHeight = h;
}

void Device9Proxy::StashBackBufferReference()
{
    ReleaseBackBufferReference();
    if (!m_real) return;
    // GetBackBuffer adds a ref; we hold that ref until Reset/destroy.
    m_real->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pTrackedBackBuffer);
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
    return m_real->QueryInterface(riid, ppvObj);
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
    // Direct Mode contract: the doubled back-buffer holds both eyes side by
    // side after the game's two render passes. Present forwards the full
    // 2WxH surface so a downstream weaver / 3D Vision driver can compose
    // the stereo image. (Without a weaver hooked up, Windows will just
    // squash the SBS image into the window — that's expected at this stage;
    // the buffer layout is correct for whoever picks it up next.)
    return m_real->Present(sr, dr, h, d);
}
HRESULT Device9Proxy::GetBackBuffer(UINT iSC, UINT iBB, D3DBACKBUFFER_TYPE T, IDirect3DSurface9** pp) { return m_real->GetBackBuffer(iSC, iBB, T, pp); }
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
    if (FAILED(hr)) return hr;

    // The actual Direct Mode magic: when game binds the doubled back buffer
    // as RT0, clamp the viewport to the active eye's half. Game-set
    // viewports (everything else) pass through unchanged.
    //
    // SetRenderTarget(0, ...) resets the viewport to full RT — i.e. the
    // doubled 2W width — so we have to overwrite it here, after the bind.
    if (i == 0 && p == m_pTrackedBackBuffer && m_logicalWidth > 0 && m_logicalHeight > 0)
    {
        const int eye = GetActiveEye();
        D3DVIEWPORT9 vp;
        vp.X      = (eye == kEyeRight) ? m_logicalWidth : 0;
        vp.Y      = 0;
        vp.Width  = m_logicalWidth;
        vp.Height = m_logicalHeight;
        vp.MinZ   = 0.0f;
        vp.MaxZ   = 1.0f;
        m_real->SetViewport(&vp);
    }
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
    // See Present() — full doubled surface goes downstream untouched.
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
