/* NvDirectMode opengl32 - per-eye FBO state
 *
 * Two FBOs (one per eye) at the game's perceived window size, sharing a
 * single depth/stencil renderbuffer. When the game binds the default
 * framebuffer (FB 0), our hook redirects the bind to the FBO of the
 * currently-active eye (read from NvApiProxy via Wiz3D_GetActiveEye).
 *
 * Adapted from OGL-3DVision-Wrapper by Octavian "Helifax" Vasilovici (MIT).
 * See THIRD_PARTY_NOTICES for attribution.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

namespace NvDirectMode
{

// Resolve the GL extension functions we need (FBO ops + blit). Called once,
// after wglMakeCurrent has produced an active context. Uses orig_wglGetProcAddress
// (held in dllmain.cpp) to look up the extension entry points.
//
// Returns true if all required entry points were found.
bool ResolveFboExtensions();

// Allocate (or re-allocate at new size) the per-eye FBO pair on the current
// context. Width/Height are the game's perceived window dims.
//
// Returns true on success. On failure, EyeFbosBind / Blit will no-op so the
// proxy degrades to passthrough.
bool EyeFbosCreate(int width, int height);

// Tear down the FBO pair. Called from wglDeleteContext / DLL detach.
void EyeFbosDestroy();

// Bind the FBO for the active eye (LEFT/RIGHT). On MONO, binds the real
// default framebuffer (id 0). Called from sys_glBindFramebuffer when game
// asks for FB 0.
void EyeFbosBindForActiveEye(GLenum target);

// At wglSwapBuffers time, blit one eye's FBO color attachment into the real
// default framebuffer (id 0), then real swap. Called from sys_wglSwapBuffers.
//
// Stage 1b-iii default: blit LEFT half only — game looks normal on a
// non-stereo monitor. A future SR weaver path could blit both halves.
void EyeFbosBlitToDefault();

// Identity check: is this id one of our internal FBOs? Used by
// sys_glGetIntegerv(GL_FRAMEBUFFER_BINDING) to lie back "0" so the game's
// save-and-restore-binding pattern doesn't lock onto our internal id.
bool IsOurFboId(GLuint id);

// Current size — used by glViewport sanity / future resize checks.
int  GetFboWidth();
int  GetFboHeight();
bool IsFboReady();

} // namespace NvDirectMode
