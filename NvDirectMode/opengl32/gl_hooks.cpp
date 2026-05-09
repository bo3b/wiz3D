/* NvDirectMode opengl32 - WGL/GL hook implementations
 *
 * Hook strategy adapted from OGL-3DVision-Wrapper by Helifax (MIT, 2015).
 * See fbo_state.cpp / THIRD_PARTY_NOTICES.txt for full attribution.
 */

#include "gl_hooks.h"
#include "fbo_state.h"
#include "eye_state.h"

#include <string.h>

#ifndef GL_FRAMEBUFFER_BINDING
#define GL_FRAMEBUFFER_BINDING            0x8CA6
#endif

extern "C" pfnWglGetProcAddress    orig_wglGetProcAddress  = nullptr;
extern "C" PFN_glBindFramebuffer_t orig_glBindFramebuffer  = nullptr;

// Indices into g_pOrigGL[] from dllmain.cpp's g_glExportNames table.
// (The array is populated by LoadRealOpenGL32 at DLL_PROCESS_ATTACH; we
// borrow specific entries here so our overrides can call the real fns.)
//
// If g_glExportNames in dllmain.cpp ever gets reordered, these must be
// kept in sync — see the static_assert pattern below.
constexpr int IDX_glGetIntegerv      = 111;
constexpr int IDX_wglGetProcAddress  = 355;
constexpr int IDX_wglMakeCurrent     = 356;
constexpr int IDX_wglSwapBuffers     = 361;

typedef void   (APIENTRY *PFN_glGetIntegerv)(GLenum, GLint*);
typedef BOOL   (WINAPI   *PFN_wglMakeCurrent)(HDC, HGLRC);
typedef BOOL   (WINAPI   *PFN_wglSwapBuffers)(HDC);

namespace
{
    // Cached real-fn pointers (lazy init from g_pOrigGL[] on first hook hit).
    PFN_glGetIntegerv      pReal_glGetIntegerv     = nullptr;
    PFN_wglMakeCurrent     pReal_wglMakeCurrent    = nullptr;
    PFN_wglSwapBuffers     pReal_wglSwapBuffers    = nullptr;

    void EnsureReals()
    {
        if (!pReal_glGetIntegerv)    pReal_glGetIntegerv    = (PFN_glGetIntegerv)    g_pOrigGL[IDX_glGetIntegerv];
        if (!pReal_wglMakeCurrent)   pReal_wglMakeCurrent   = (PFN_wglMakeCurrent)   g_pOrigGL[IDX_wglMakeCurrent];
        if (!pReal_wglSwapBuffers)   pReal_wglSwapBuffers   = (PFN_wglSwapBuffers)   g_pOrigGL[IDX_wglSwapBuffers];
        if (!orig_wglGetProcAddress) orig_wglGetProcAddress = (pfnWglGetProcAddress) g_pOrigGL[IDX_wglGetProcAddress];
    }

    // Returns the window's current client area size (W,H). Returns false if
    // the DC's window can't be resolved.
    bool GetWindowClientSize(HDC hdc, int& outW, int& outH)
    {
        HWND hwnd = WindowFromDC(hdc);
        if (!hwnd) return false;
        RECT rc;
        if (!GetClientRect(hwnd, &rc)) return false;
        outW = (int)(rc.right - rc.left);
        outH = (int)(rc.bottom - rc.top);
        return outW > 0 && outH > 0;
    }
}

// ---------------------------------------------------------------------------
// sys_glBindFramebuffer: returned by wglGetProcAddress for glBindFramebuffer,
// glBindFramebufferEXT, glBindFramebufferARB. When the game asks to bind FB
// 0 AND WrapDevices is enabled, redirect to the active-eye internal FBO.
// Otherwise plain passthrough to the real fn.
// ---------------------------------------------------------------------------
extern "C" void APIENTRY sys_glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    if (framebuffer == 0 && NvDM_WrapDevices())
    {
        NvDirectMode::EyeFbosBindForActiveEye(target);
        return;
    }
    if (orig_glBindFramebuffer) orig_glBindFramebuffer(target, framebuffer);
}

// ---------------------------------------------------------------------------
// Exported overrides — these REPLACE the corresponding naked-jmp thunks that
// dllmain.cpp would otherwise generate for wglMakeCurrent / wglSwapBuffers /
// wglGetProcAddress / glGetIntegerv. The .def file routes the export name to
// whichever symbol is provided; ours wins because we provide a real impl.
// ---------------------------------------------------------------------------

extern "C" __declspec(dllexport) BOOL WINAPI wglMakeCurrent(HDC hdc, HGLRC hglrc)
{
    EnsureReals();
    BOOL ok = pReal_wglMakeCurrent ? pReal_wglMakeCurrent(hdc, hglrc) : FALSE;
    if (ok && hdc && hglrc && NvDM_WrapDevices())
    {
        // Lazy-create / re-create the per-eye FBO pair at the window's
        // current client size. EyeFbosCreate is idempotent — same size = no-op.
        int w = 0, h = 0;
        if (GetWindowClientSize(hdc, w, h))
            NvDirectMode::EyeFbosCreate(w, h);
    }
    return ok;
}

extern "C" __declspec(dllexport) BOOL WINAPI wglSwapBuffers(HDC hdc)
{
    EnsureReals();
    // Blit active-eye FBO color to the real default framebuffer before the
    // OS-level swap, but only if FBO routing is on (WrapDevices=1). With
    // WrapDevices=0 the FBO was never created, so just real swap.
    if (NvDM_WrapDevices())
        NvDirectMode::EyeFbosBlitToDefault();
    return pReal_wglSwapBuffers ? pReal_wglSwapBuffers(hdc) : FALSE;
}

// Same C4273 mitigation as glGetIntegerv below; wingdi.h's PROC return type
// declaration triggers it too on Win32 builds.
#pragma warning(suppress: 4273)
extern "C" __declspec(dllexport) PROC WINAPI wglGetProcAddress(LPCSTR ProcName)
{
    EnsureReals();
    if (!orig_wglGetProcAddress || !ProcName) return nullptr;

    // Cache the real glBindFramebuffer the first time the game asks for it
    // (or any of its EXT/ARB aliases — they're the same underlying fn).
    if (!strcmp(ProcName, "glBindFramebuffer")    ||
        !strcmp(ProcName, "glBindFramebufferEXT") ||
        !strcmp(ProcName, "glBindFramebufferARB"))
    {
        if (!orig_glBindFramebuffer)
            orig_glBindFramebuffer = (PFN_glBindFramebuffer_t)orig_wglGetProcAddress(ProcName);
        return (PROC)sys_glBindFramebuffer;
    }
    return orig_wglGetProcAddress(ProcName);
}

// gl.h declares glGetIntegerv with WINGDIAPI which evaluates to dllimport
// in this TU's include context — silence the consequent C4273 (linkage
// mismatch). Our .def + dllexport here is what actually wins.
#pragma warning(suppress: 4273)
extern "C" __declspec(dllexport) void WINAPI glGetIntegerv(GLenum pname, GLint* params)
{
    EnsureReals();
    if (!pReal_glGetIntegerv) return;
    if (pname == GL_FRAMEBUFFER_BINDING && params)
    {
        GLint actual = 0;
        pReal_glGetIntegerv(GL_FRAMEBUFFER_BINDING, &actual);
        // Lie back "0" when one of our internal FBOs is bound, so a game's
        // save-current-binding-and-restore pattern doesn't latch onto our id.
        if (NvDirectMode::IsOurFboId((GLuint)actual))
            *params = 0;
        else
            *params = actual;
        return;
    }
    pReal_glGetIntegerv(pname, params);
}
