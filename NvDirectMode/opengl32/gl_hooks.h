/* NvDirectMode opengl32 - WGL/GL hook implementations
 *
 * The function names here that are exported by name (wglMakeCurrent,
 * wglSwapBuffers, wglGetProcAddress, glGetIntegerv) replace the naked-jmp
 * thunks in dllmain.cpp / thunks_x64.asm — those four entries are removed
 * from the passthrough thunk list so that our exported symbols (matching
 * the names in opengl32.def) provide the implementation instead.
 *
 * Hook strategy adapted from OGL-3DVision-Wrapper (MIT, see fbo_state.cpp).
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

// Globals defined in dllmain.cpp, accessed from gl_hooks.cpp + fbo_state.cpp.
extern "C" HMODULE g_hRealOpenGL32;
extern "C" FARPROC g_pOrigGL[368];

// Real wglGetProcAddress pointer — resolved lazily on first hook call.
typedef PROC (WINAPI* pfnWglGetProcAddress)(LPCSTR);
extern "C" pfnWglGetProcAddress orig_wglGetProcAddress;

// Real glBindFramebuffer pointer (resolved on first wglGetProcAddress hit).
// Used by sys_glBindFramebuffer for the non-redirect passthrough case.
typedef void (APIENTRY *PFN_glBindFramebuffer_t)(GLenum, GLuint);
extern "C" PFN_glBindFramebuffer_t orig_glBindFramebuffer;

// WrapDevices flag from 3DVision_Config.xml — 0 disables FBO redirect
// so the proxy is pure-passthrough except for diagnostic logging.
extern "C" int NvDM_WrapDevices();
