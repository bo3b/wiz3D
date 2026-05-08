; wiz3D opengl32.dll proxy - x64 naked jmp thunks (MASM64)
; Auto-generated from opengl32.dll export list (368 functions)
; Each thunk loads the real function pointer from g_pOrigGL[idx] and jumps.

EXTERN g_pOrigGL:QWORD

.CODE

GlmfBeginGlsBlock PROC
    mov rax, QWORD PTR [g_pOrigGL + 0]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
GlmfBeginGlsBlock ENDP

GlmfCloseMetaFile PROC
    mov rax, QWORD PTR [g_pOrigGL + 8]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
GlmfCloseMetaFile ENDP

GlmfEndGlsBlock PROC
    mov rax, QWORD PTR [g_pOrigGL + 16]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
GlmfEndGlsBlock ENDP

GlmfEndPlayback PROC
    mov rax, QWORD PTR [g_pOrigGL + 24]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
GlmfEndPlayback ENDP

GlmfInitPlayback PROC
    mov rax, QWORD PTR [g_pOrigGL + 32]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
GlmfInitPlayback ENDP

GlmfPlayGlsRecord PROC
    mov rax, QWORD PTR [g_pOrigGL + 40]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
GlmfPlayGlsRecord ENDP

glAccum PROC
    mov rax, QWORD PTR [g_pOrigGL + 48]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glAccum ENDP

glAlphaFunc PROC
    mov rax, QWORD PTR [g_pOrigGL + 56]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glAlphaFunc ENDP

glAreTexturesResident PROC
    mov rax, QWORD PTR [g_pOrigGL + 64]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glAreTexturesResident ENDP

glArrayElement PROC
    mov rax, QWORD PTR [g_pOrigGL + 72]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glArrayElement ENDP

glBegin PROC
    mov rax, QWORD PTR [g_pOrigGL + 80]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glBegin ENDP

glBindTexture PROC
    mov rax, QWORD PTR [g_pOrigGL + 88]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glBindTexture ENDP

glBitmap PROC
    mov rax, QWORD PTR [g_pOrigGL + 96]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glBitmap ENDP

glBlendFunc PROC
    mov rax, QWORD PTR [g_pOrigGL + 104]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glBlendFunc ENDP

glCallList PROC
    mov rax, QWORD PTR [g_pOrigGL + 112]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glCallList ENDP

glCallLists PROC
    mov rax, QWORD PTR [g_pOrigGL + 120]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glCallLists ENDP

glClear PROC
    mov rax, QWORD PTR [g_pOrigGL + 128]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glClear ENDP

glClearAccum PROC
    mov rax, QWORD PTR [g_pOrigGL + 136]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glClearAccum ENDP

glClearColor PROC
    mov rax, QWORD PTR [g_pOrigGL + 144]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glClearColor ENDP

glClearDepth PROC
    mov rax, QWORD PTR [g_pOrigGL + 152]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glClearDepth ENDP

glClearIndex PROC
    mov rax, QWORD PTR [g_pOrigGL + 160]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glClearIndex ENDP

glClearStencil PROC
    mov rax, QWORD PTR [g_pOrigGL + 168]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glClearStencil ENDP

glClipPlane PROC
    mov rax, QWORD PTR [g_pOrigGL + 176]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glClipPlane ENDP

glColor3b PROC
    mov rax, QWORD PTR [g_pOrigGL + 184]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3b ENDP

glColor3bv PROC
    mov rax, QWORD PTR [g_pOrigGL + 192]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3bv ENDP

glColor3d PROC
    mov rax, QWORD PTR [g_pOrigGL + 200]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3d ENDP

glColor3dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 208]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3dv ENDP

glColor3f PROC
    mov rax, QWORD PTR [g_pOrigGL + 216]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3f ENDP

glColor3fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 224]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3fv ENDP

glColor3i PROC
    mov rax, QWORD PTR [g_pOrigGL + 232]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3i ENDP

glColor3iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 240]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3iv ENDP

glColor3s PROC
    mov rax, QWORD PTR [g_pOrigGL + 248]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3s ENDP

glColor3sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 256]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3sv ENDP

glColor3ub PROC
    mov rax, QWORD PTR [g_pOrigGL + 264]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3ub ENDP

glColor3ubv PROC
    mov rax, QWORD PTR [g_pOrigGL + 272]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3ubv ENDP

glColor3ui PROC
    mov rax, QWORD PTR [g_pOrigGL + 280]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3ui ENDP

glColor3uiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 288]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3uiv ENDP

glColor3us PROC
    mov rax, QWORD PTR [g_pOrigGL + 296]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3us ENDP

glColor3usv PROC
    mov rax, QWORD PTR [g_pOrigGL + 304]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor3usv ENDP

glColor4b PROC
    mov rax, QWORD PTR [g_pOrigGL + 312]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4b ENDP

glColor4bv PROC
    mov rax, QWORD PTR [g_pOrigGL + 320]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4bv ENDP

glColor4d PROC
    mov rax, QWORD PTR [g_pOrigGL + 328]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4d ENDP

glColor4dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 336]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4dv ENDP

glColor4f PROC
    mov rax, QWORD PTR [g_pOrigGL + 344]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4f ENDP

glColor4fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 352]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4fv ENDP

glColor4i PROC
    mov rax, QWORD PTR [g_pOrigGL + 360]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4i ENDP

glColor4iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 368]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4iv ENDP

glColor4s PROC
    mov rax, QWORD PTR [g_pOrigGL + 376]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4s ENDP

glColor4sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 384]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4sv ENDP

glColor4ub PROC
    mov rax, QWORD PTR [g_pOrigGL + 392]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4ub ENDP

glColor4ubv PROC
    mov rax, QWORD PTR [g_pOrigGL + 400]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4ubv ENDP

glColor4ui PROC
    mov rax, QWORD PTR [g_pOrigGL + 408]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4ui ENDP

glColor4uiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 416]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4uiv ENDP

glColor4us PROC
    mov rax, QWORD PTR [g_pOrigGL + 424]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4us ENDP

glColor4usv PROC
    mov rax, QWORD PTR [g_pOrigGL + 432]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColor4usv ENDP

glColorMask PROC
    mov rax, QWORD PTR [g_pOrigGL + 440]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColorMask ENDP

glColorMaterial PROC
    mov rax, QWORD PTR [g_pOrigGL + 448]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColorMaterial ENDP

glColorPointer PROC
    mov rax, QWORD PTR [g_pOrigGL + 456]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glColorPointer ENDP

glCopyPixels PROC
    mov rax, QWORD PTR [g_pOrigGL + 464]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glCopyPixels ENDP

glCopyTexImage1D PROC
    mov rax, QWORD PTR [g_pOrigGL + 472]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glCopyTexImage1D ENDP

glCopyTexImage2D PROC
    mov rax, QWORD PTR [g_pOrigGL + 480]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glCopyTexImage2D ENDP

glCopyTexSubImage1D PROC
    mov rax, QWORD PTR [g_pOrigGL + 488]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glCopyTexSubImage1D ENDP

glCopyTexSubImage2D PROC
    mov rax, QWORD PTR [g_pOrigGL + 496]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glCopyTexSubImage2D ENDP

glCullFace PROC
    mov rax, QWORD PTR [g_pOrigGL + 504]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glCullFace ENDP

glDebugEntry PROC
    mov rax, QWORD PTR [g_pOrigGL + 512]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDebugEntry ENDP

glDeleteLists PROC
    mov rax, QWORD PTR [g_pOrigGL + 520]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDeleteLists ENDP

glDeleteTextures PROC
    mov rax, QWORD PTR [g_pOrigGL + 528]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDeleteTextures ENDP

glDepthFunc PROC
    mov rax, QWORD PTR [g_pOrigGL + 536]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDepthFunc ENDP

glDepthMask PROC
    mov rax, QWORD PTR [g_pOrigGL + 544]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDepthMask ENDP

glDepthRange PROC
    mov rax, QWORD PTR [g_pOrigGL + 552]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDepthRange ENDP

glDisable PROC
    mov rax, QWORD PTR [g_pOrigGL + 560]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDisable ENDP

glDisableClientState PROC
    mov rax, QWORD PTR [g_pOrigGL + 568]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDisableClientState ENDP

glDrawArrays PROC
    mov rax, QWORD PTR [g_pOrigGL + 576]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDrawArrays ENDP

glDrawBuffer PROC
    mov rax, QWORD PTR [g_pOrigGL + 584]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDrawBuffer ENDP

glDrawElements PROC
    mov rax, QWORD PTR [g_pOrigGL + 592]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDrawElements ENDP

glDrawPixels PROC
    mov rax, QWORD PTR [g_pOrigGL + 600]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glDrawPixels ENDP

glEdgeFlag PROC
    mov rax, QWORD PTR [g_pOrigGL + 608]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEdgeFlag ENDP

glEdgeFlagPointer PROC
    mov rax, QWORD PTR [g_pOrigGL + 616]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEdgeFlagPointer ENDP

glEdgeFlagv PROC
    mov rax, QWORD PTR [g_pOrigGL + 624]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEdgeFlagv ENDP

glEnable PROC
    mov rax, QWORD PTR [g_pOrigGL + 632]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEnable ENDP

glEnableClientState PROC
    mov rax, QWORD PTR [g_pOrigGL + 640]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEnableClientState ENDP

glEnd PROC
    mov rax, QWORD PTR [g_pOrigGL + 648]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEnd ENDP

glEndList PROC
    mov rax, QWORD PTR [g_pOrigGL + 656]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEndList ENDP

glEvalCoord1d PROC
    mov rax, QWORD PTR [g_pOrigGL + 664]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalCoord1d ENDP

glEvalCoord1dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 672]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalCoord1dv ENDP

glEvalCoord1f PROC
    mov rax, QWORD PTR [g_pOrigGL + 680]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalCoord1f ENDP

glEvalCoord1fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 688]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalCoord1fv ENDP

glEvalCoord2d PROC
    mov rax, QWORD PTR [g_pOrigGL + 696]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalCoord2d ENDP

glEvalCoord2dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 704]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalCoord2dv ENDP

glEvalCoord2f PROC
    mov rax, QWORD PTR [g_pOrigGL + 712]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalCoord2f ENDP

glEvalCoord2fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 720]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalCoord2fv ENDP

glEvalMesh1 PROC
    mov rax, QWORD PTR [g_pOrigGL + 728]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalMesh1 ENDP

glEvalMesh2 PROC
    mov rax, QWORD PTR [g_pOrigGL + 736]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalMesh2 ENDP

glEvalPoint1 PROC
    mov rax, QWORD PTR [g_pOrigGL + 744]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalPoint1 ENDP

glEvalPoint2 PROC
    mov rax, QWORD PTR [g_pOrigGL + 752]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glEvalPoint2 ENDP

glFeedbackBuffer PROC
    mov rax, QWORD PTR [g_pOrigGL + 760]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glFeedbackBuffer ENDP

glFinish PROC
    mov rax, QWORD PTR [g_pOrigGL + 768]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glFinish ENDP

glFlush PROC
    mov rax, QWORD PTR [g_pOrigGL + 776]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glFlush ENDP

glFogf PROC
    mov rax, QWORD PTR [g_pOrigGL + 784]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glFogf ENDP

glFogfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 792]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glFogfv ENDP

glFogi PROC
    mov rax, QWORD PTR [g_pOrigGL + 800]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glFogi ENDP

glFogiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 808]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glFogiv ENDP

glFrontFace PROC
    mov rax, QWORD PTR [g_pOrigGL + 816]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glFrontFace ENDP

glFrustum PROC
    mov rax, QWORD PTR [g_pOrigGL + 824]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glFrustum ENDP

glGenLists PROC
    mov rax, QWORD PTR [g_pOrigGL + 832]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGenLists ENDP

glGenTextures PROC
    mov rax, QWORD PTR [g_pOrigGL + 840]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGenTextures ENDP

glGetBooleanv PROC
    mov rax, QWORD PTR [g_pOrigGL + 848]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetBooleanv ENDP

glGetClipPlane PROC
    mov rax, QWORD PTR [g_pOrigGL + 856]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetClipPlane ENDP

glGetDoublev PROC
    mov rax, QWORD PTR [g_pOrigGL + 864]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetDoublev ENDP

glGetError PROC
    mov rax, QWORD PTR [g_pOrigGL + 872]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetError ENDP

glGetFloatv PROC
    mov rax, QWORD PTR [g_pOrigGL + 880]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetFloatv ENDP

; glGetIntegerv (idx 111, byte offset 888) - overridden by gl_hooks.cpp
;     to lie about GL_FRAMEBUFFER_BINDING when our internal FBO is bound.
;     Original PROC removed so the linker uses our exported impl.

glGetLightfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 896]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetLightfv ENDP

glGetLightiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 904]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetLightiv ENDP

glGetMapdv PROC
    mov rax, QWORD PTR [g_pOrigGL + 912]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetMapdv ENDP

glGetMapfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 920]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetMapfv ENDP

glGetMapiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 928]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetMapiv ENDP

glGetMaterialfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 936]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetMaterialfv ENDP

glGetMaterialiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 944]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetMaterialiv ENDP

glGetPixelMapfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 952]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetPixelMapfv ENDP

glGetPixelMapuiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 960]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetPixelMapuiv ENDP

glGetPixelMapusv PROC
    mov rax, QWORD PTR [g_pOrigGL + 968]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetPixelMapusv ENDP

glGetPointerv PROC
    mov rax, QWORD PTR [g_pOrigGL + 976]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetPointerv ENDP

glGetPolygonStipple PROC
    mov rax, QWORD PTR [g_pOrigGL + 984]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetPolygonStipple ENDP

glGetString PROC
    mov rax, QWORD PTR [g_pOrigGL + 992]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetString ENDP

glGetTexEnvfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1000]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetTexEnvfv ENDP

glGetTexEnviv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1008]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetTexEnviv ENDP

glGetTexGendv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1016]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetTexGendv ENDP

glGetTexGenfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1024]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetTexGenfv ENDP

glGetTexGeniv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1032]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetTexGeniv ENDP

glGetTexImage PROC
    mov rax, QWORD PTR [g_pOrigGL + 1040]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetTexImage ENDP

glGetTexLevelParameterfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1048]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetTexLevelParameterfv ENDP

glGetTexLevelParameteriv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1056]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetTexLevelParameteriv ENDP

glGetTexParameterfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1064]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetTexParameterfv ENDP

glGetTexParameteriv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1072]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glGetTexParameteriv ENDP

glHint PROC
    mov rax, QWORD PTR [g_pOrigGL + 1080]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glHint ENDP

glIndexMask PROC
    mov rax, QWORD PTR [g_pOrigGL + 1088]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexMask ENDP

glIndexPointer PROC
    mov rax, QWORD PTR [g_pOrigGL + 1096]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexPointer ENDP

glIndexd PROC
    mov rax, QWORD PTR [g_pOrigGL + 1104]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexd ENDP

glIndexdv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1112]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexdv ENDP

glIndexf PROC
    mov rax, QWORD PTR [g_pOrigGL + 1120]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexf ENDP

glIndexfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1128]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexfv ENDP

glIndexi PROC
    mov rax, QWORD PTR [g_pOrigGL + 1136]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexi ENDP

glIndexiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1144]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexiv ENDP

glIndexs PROC
    mov rax, QWORD PTR [g_pOrigGL + 1152]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexs ENDP

glIndexsv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1160]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexsv ENDP

glIndexub PROC
    mov rax, QWORD PTR [g_pOrigGL + 1168]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexub ENDP

glIndexubv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1176]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIndexubv ENDP

glInitNames PROC
    mov rax, QWORD PTR [g_pOrigGL + 1184]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glInitNames ENDP

glInterleavedArrays PROC
    mov rax, QWORD PTR [g_pOrigGL + 1192]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glInterleavedArrays ENDP

glIsEnabled PROC
    mov rax, QWORD PTR [g_pOrigGL + 1200]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIsEnabled ENDP

glIsList PROC
    mov rax, QWORD PTR [g_pOrigGL + 1208]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIsList ENDP

glIsTexture PROC
    mov rax, QWORD PTR [g_pOrigGL + 1216]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glIsTexture ENDP

glLightModelf PROC
    mov rax, QWORD PTR [g_pOrigGL + 1224]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLightModelf ENDP

glLightModelfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1232]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLightModelfv ENDP

glLightModeli PROC
    mov rax, QWORD PTR [g_pOrigGL + 1240]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLightModeli ENDP

glLightModeliv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1248]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLightModeliv ENDP

glLightf PROC
    mov rax, QWORD PTR [g_pOrigGL + 1256]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLightf ENDP

glLightfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1264]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLightfv ENDP

glLighti PROC
    mov rax, QWORD PTR [g_pOrigGL + 1272]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLighti ENDP

glLightiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1280]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLightiv ENDP

glLineStipple PROC
    mov rax, QWORD PTR [g_pOrigGL + 1288]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLineStipple ENDP

glLineWidth PROC
    mov rax, QWORD PTR [g_pOrigGL + 1296]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLineWidth ENDP

glListBase PROC
    mov rax, QWORD PTR [g_pOrigGL + 1304]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glListBase ENDP

glLoadIdentity PROC
    mov rax, QWORD PTR [g_pOrigGL + 1312]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLoadIdentity ENDP

glLoadMatrixd PROC
    mov rax, QWORD PTR [g_pOrigGL + 1320]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLoadMatrixd ENDP

glLoadMatrixf PROC
    mov rax, QWORD PTR [g_pOrigGL + 1328]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLoadMatrixf ENDP

glLoadName PROC
    mov rax, QWORD PTR [g_pOrigGL + 1336]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLoadName ENDP

glLogicOp PROC
    mov rax, QWORD PTR [g_pOrigGL + 1344]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glLogicOp ENDP

glMap1d PROC
    mov rax, QWORD PTR [g_pOrigGL + 1352]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMap1d ENDP

glMap1f PROC
    mov rax, QWORD PTR [g_pOrigGL + 1360]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMap1f ENDP

glMap2d PROC
    mov rax, QWORD PTR [g_pOrigGL + 1368]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMap2d ENDP

glMap2f PROC
    mov rax, QWORD PTR [g_pOrigGL + 1376]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMap2f ENDP

glMapGrid1d PROC
    mov rax, QWORD PTR [g_pOrigGL + 1384]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMapGrid1d ENDP

glMapGrid1f PROC
    mov rax, QWORD PTR [g_pOrigGL + 1392]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMapGrid1f ENDP

glMapGrid2d PROC
    mov rax, QWORD PTR [g_pOrigGL + 1400]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMapGrid2d ENDP

glMapGrid2f PROC
    mov rax, QWORD PTR [g_pOrigGL + 1408]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMapGrid2f ENDP

glMaterialf PROC
    mov rax, QWORD PTR [g_pOrigGL + 1416]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMaterialf ENDP

glMaterialfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1424]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMaterialfv ENDP

glMateriali PROC
    mov rax, QWORD PTR [g_pOrigGL + 1432]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMateriali ENDP

glMaterialiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1440]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMaterialiv ENDP

glMatrixMode PROC
    mov rax, QWORD PTR [g_pOrigGL + 1448]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMatrixMode ENDP

glMultMatrixd PROC
    mov rax, QWORD PTR [g_pOrigGL + 1456]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMultMatrixd ENDP

glMultMatrixf PROC
    mov rax, QWORD PTR [g_pOrigGL + 1464]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glMultMatrixf ENDP

glNewList PROC
    mov rax, QWORD PTR [g_pOrigGL + 1472]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNewList ENDP

glNormal3b PROC
    mov rax, QWORD PTR [g_pOrigGL + 1480]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNormal3b ENDP

glNormal3bv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1488]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNormal3bv ENDP

glNormal3d PROC
    mov rax, QWORD PTR [g_pOrigGL + 1496]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNormal3d ENDP

glNormal3dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1504]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNormal3dv ENDP

glNormal3f PROC
    mov rax, QWORD PTR [g_pOrigGL + 1512]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNormal3f ENDP

glNormal3fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1520]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNormal3fv ENDP

glNormal3i PROC
    mov rax, QWORD PTR [g_pOrigGL + 1528]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNormal3i ENDP

glNormal3iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1536]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNormal3iv ENDP

glNormal3s PROC
    mov rax, QWORD PTR [g_pOrigGL + 1544]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNormal3s ENDP

glNormal3sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1552]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNormal3sv ENDP

glNormalPointer PROC
    mov rax, QWORD PTR [g_pOrigGL + 1560]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glNormalPointer ENDP

glOrtho PROC
    mov rax, QWORD PTR [g_pOrigGL + 1568]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glOrtho ENDP

glPassThrough PROC
    mov rax, QWORD PTR [g_pOrigGL + 1576]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPassThrough ENDP

glPixelMapfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1584]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPixelMapfv ENDP

glPixelMapuiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1592]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPixelMapuiv ENDP

glPixelMapusv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1600]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPixelMapusv ENDP

glPixelStoref PROC
    mov rax, QWORD PTR [g_pOrigGL + 1608]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPixelStoref ENDP

glPixelStorei PROC
    mov rax, QWORD PTR [g_pOrigGL + 1616]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPixelStorei ENDP

glPixelTransferf PROC
    mov rax, QWORD PTR [g_pOrigGL + 1624]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPixelTransferf ENDP

glPixelTransferi PROC
    mov rax, QWORD PTR [g_pOrigGL + 1632]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPixelTransferi ENDP

glPixelZoom PROC
    mov rax, QWORD PTR [g_pOrigGL + 1640]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPixelZoom ENDP

glPointSize PROC
    mov rax, QWORD PTR [g_pOrigGL + 1648]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPointSize ENDP

glPolygonMode PROC
    mov rax, QWORD PTR [g_pOrigGL + 1656]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPolygonMode ENDP

glPolygonOffset PROC
    mov rax, QWORD PTR [g_pOrigGL + 1664]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPolygonOffset ENDP

glPolygonStipple PROC
    mov rax, QWORD PTR [g_pOrigGL + 1672]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPolygonStipple ENDP

glPopAttrib PROC
    mov rax, QWORD PTR [g_pOrigGL + 1680]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPopAttrib ENDP

glPopClientAttrib PROC
    mov rax, QWORD PTR [g_pOrigGL + 1688]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPopClientAttrib ENDP

glPopMatrix PROC
    mov rax, QWORD PTR [g_pOrigGL + 1696]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPopMatrix ENDP

glPopName PROC
    mov rax, QWORD PTR [g_pOrigGL + 1704]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPopName ENDP

glPrioritizeTextures PROC
    mov rax, QWORD PTR [g_pOrigGL + 1712]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPrioritizeTextures ENDP

glPushAttrib PROC
    mov rax, QWORD PTR [g_pOrigGL + 1720]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPushAttrib ENDP

glPushClientAttrib PROC
    mov rax, QWORD PTR [g_pOrigGL + 1728]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPushClientAttrib ENDP

glPushMatrix PROC
    mov rax, QWORD PTR [g_pOrigGL + 1736]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPushMatrix ENDP

glPushName PROC
    mov rax, QWORD PTR [g_pOrigGL + 1744]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glPushName ENDP

glRasterPos2d PROC
    mov rax, QWORD PTR [g_pOrigGL + 1752]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos2d ENDP

glRasterPos2dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1760]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos2dv ENDP

glRasterPos2f PROC
    mov rax, QWORD PTR [g_pOrigGL + 1768]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos2f ENDP

glRasterPos2fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1776]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos2fv ENDP

glRasterPos2i PROC
    mov rax, QWORD PTR [g_pOrigGL + 1784]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos2i ENDP

glRasterPos2iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1792]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos2iv ENDP

glRasterPos2s PROC
    mov rax, QWORD PTR [g_pOrigGL + 1800]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos2s ENDP

glRasterPos2sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1808]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos2sv ENDP

glRasterPos3d PROC
    mov rax, QWORD PTR [g_pOrigGL + 1816]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos3d ENDP

glRasterPos3dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1824]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos3dv ENDP

glRasterPos3f PROC
    mov rax, QWORD PTR [g_pOrigGL + 1832]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos3f ENDP

glRasterPos3fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1840]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos3fv ENDP

glRasterPos3i PROC
    mov rax, QWORD PTR [g_pOrigGL + 1848]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos3i ENDP

glRasterPos3iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1856]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos3iv ENDP

glRasterPos3s PROC
    mov rax, QWORD PTR [g_pOrigGL + 1864]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos3s ENDP

glRasterPos3sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1872]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos3sv ENDP

glRasterPos4d PROC
    mov rax, QWORD PTR [g_pOrigGL + 1880]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos4d ENDP

glRasterPos4dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1888]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos4dv ENDP

glRasterPos4f PROC
    mov rax, QWORD PTR [g_pOrigGL + 1896]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos4f ENDP

glRasterPos4fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1904]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos4fv ENDP

glRasterPos4i PROC
    mov rax, QWORD PTR [g_pOrigGL + 1912]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos4i ENDP

glRasterPos4iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1920]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos4iv ENDP

glRasterPos4s PROC
    mov rax, QWORD PTR [g_pOrigGL + 1928]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos4s ENDP

glRasterPos4sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1936]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRasterPos4sv ENDP

glReadBuffer PROC
    mov rax, QWORD PTR [g_pOrigGL + 1944]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glReadBuffer ENDP

glReadPixels PROC
    mov rax, QWORD PTR [g_pOrigGL + 1952]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glReadPixels ENDP

glRectd PROC
    mov rax, QWORD PTR [g_pOrigGL + 1960]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRectd ENDP

glRectdv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1968]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRectdv ENDP

glRectf PROC
    mov rax, QWORD PTR [g_pOrigGL + 1976]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRectf ENDP

glRectfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 1984]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRectfv ENDP

glRecti PROC
    mov rax, QWORD PTR [g_pOrigGL + 1992]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRecti ENDP

glRectiv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2000]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRectiv ENDP

glRects PROC
    mov rax, QWORD PTR [g_pOrigGL + 2008]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRects ENDP

glRectsv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2016]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRectsv ENDP

glRenderMode PROC
    mov rax, QWORD PTR [g_pOrigGL + 2024]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRenderMode ENDP

glRotated PROC
    mov rax, QWORD PTR [g_pOrigGL + 2032]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRotated ENDP

glRotatef PROC
    mov rax, QWORD PTR [g_pOrigGL + 2040]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glRotatef ENDP

glScaled PROC
    mov rax, QWORD PTR [g_pOrigGL + 2048]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glScaled ENDP

glScalef PROC
    mov rax, QWORD PTR [g_pOrigGL + 2056]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glScalef ENDP

glScissor PROC
    mov rax, QWORD PTR [g_pOrigGL + 2064]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glScissor ENDP

glSelectBuffer PROC
    mov rax, QWORD PTR [g_pOrigGL + 2072]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glSelectBuffer ENDP

glShadeModel PROC
    mov rax, QWORD PTR [g_pOrigGL + 2080]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glShadeModel ENDP

glStencilFunc PROC
    mov rax, QWORD PTR [g_pOrigGL + 2088]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glStencilFunc ENDP

glStencilMask PROC
    mov rax, QWORD PTR [g_pOrigGL + 2096]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glStencilMask ENDP

glStencilOp PROC
    mov rax, QWORD PTR [g_pOrigGL + 2104]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glStencilOp ENDP

glTexCoord1d PROC
    mov rax, QWORD PTR [g_pOrigGL + 2112]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord1d ENDP

glTexCoord1dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2120]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord1dv ENDP

glTexCoord1f PROC
    mov rax, QWORD PTR [g_pOrigGL + 2128]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord1f ENDP

glTexCoord1fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2136]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord1fv ENDP

glTexCoord1i PROC
    mov rax, QWORD PTR [g_pOrigGL + 2144]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord1i ENDP

glTexCoord1iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2152]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord1iv ENDP

glTexCoord1s PROC
    mov rax, QWORD PTR [g_pOrigGL + 2160]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord1s ENDP

glTexCoord1sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2168]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord1sv ENDP

glTexCoord2d PROC
    mov rax, QWORD PTR [g_pOrigGL + 2176]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord2d ENDP

glTexCoord2dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2184]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord2dv ENDP

glTexCoord2f PROC
    mov rax, QWORD PTR [g_pOrigGL + 2192]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord2f ENDP

glTexCoord2fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2200]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord2fv ENDP

glTexCoord2i PROC
    mov rax, QWORD PTR [g_pOrigGL + 2208]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord2i ENDP

glTexCoord2iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2216]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord2iv ENDP

glTexCoord2s PROC
    mov rax, QWORD PTR [g_pOrigGL + 2224]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord2s ENDP

glTexCoord2sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2232]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord2sv ENDP

glTexCoord3d PROC
    mov rax, QWORD PTR [g_pOrigGL + 2240]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord3d ENDP

glTexCoord3dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2248]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord3dv ENDP

glTexCoord3f PROC
    mov rax, QWORD PTR [g_pOrigGL + 2256]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord3f ENDP

glTexCoord3fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2264]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord3fv ENDP

glTexCoord3i PROC
    mov rax, QWORD PTR [g_pOrigGL + 2272]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord3i ENDP

glTexCoord3iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2280]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord3iv ENDP

glTexCoord3s PROC
    mov rax, QWORD PTR [g_pOrigGL + 2288]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord3s ENDP

glTexCoord3sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2296]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord3sv ENDP

glTexCoord4d PROC
    mov rax, QWORD PTR [g_pOrigGL + 2304]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord4d ENDP

glTexCoord4dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2312]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord4dv ENDP

glTexCoord4f PROC
    mov rax, QWORD PTR [g_pOrigGL + 2320]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord4f ENDP

glTexCoord4fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2328]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord4fv ENDP

glTexCoord4i PROC
    mov rax, QWORD PTR [g_pOrigGL + 2336]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord4i ENDP

glTexCoord4iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2344]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord4iv ENDP

glTexCoord4s PROC
    mov rax, QWORD PTR [g_pOrigGL + 2352]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord4s ENDP

glTexCoord4sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2360]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoord4sv ENDP

glTexCoordPointer PROC
    mov rax, QWORD PTR [g_pOrigGL + 2368]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexCoordPointer ENDP

glTexEnvf PROC
    mov rax, QWORD PTR [g_pOrigGL + 2376]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexEnvf ENDP

glTexEnvfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2384]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexEnvfv ENDP

glTexEnvi PROC
    mov rax, QWORD PTR [g_pOrigGL + 2392]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexEnvi ENDP

glTexEnviv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2400]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexEnviv ENDP

glTexGend PROC
    mov rax, QWORD PTR [g_pOrigGL + 2408]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexGend ENDP

glTexGendv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2416]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexGendv ENDP

glTexGenf PROC
    mov rax, QWORD PTR [g_pOrigGL + 2424]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexGenf ENDP

glTexGenfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2432]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexGenfv ENDP

glTexGeni PROC
    mov rax, QWORD PTR [g_pOrigGL + 2440]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexGeni ENDP

glTexGeniv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2448]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexGeniv ENDP

glTexImage1D PROC
    mov rax, QWORD PTR [g_pOrigGL + 2456]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexImage1D ENDP

glTexImage2D PROC
    mov rax, QWORD PTR [g_pOrigGL + 2464]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexImage2D ENDP

glTexParameterf PROC
    mov rax, QWORD PTR [g_pOrigGL + 2472]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexParameterf ENDP

glTexParameterfv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2480]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexParameterfv ENDP

glTexParameteri PROC
    mov rax, QWORD PTR [g_pOrigGL + 2488]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexParameteri ENDP

glTexParameteriv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2496]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexParameteriv ENDP

glTexSubImage1D PROC
    mov rax, QWORD PTR [g_pOrigGL + 2504]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexSubImage1D ENDP

glTexSubImage2D PROC
    mov rax, QWORD PTR [g_pOrigGL + 2512]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTexSubImage2D ENDP

glTranslated PROC
    mov rax, QWORD PTR [g_pOrigGL + 2520]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTranslated ENDP

glTranslatef PROC
    mov rax, QWORD PTR [g_pOrigGL + 2528]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glTranslatef ENDP

glVertex2d PROC
    mov rax, QWORD PTR [g_pOrigGL + 2536]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex2d ENDP

glVertex2dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2544]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex2dv ENDP

glVertex2f PROC
    mov rax, QWORD PTR [g_pOrigGL + 2552]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex2f ENDP

glVertex2fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2560]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex2fv ENDP

glVertex2i PROC
    mov rax, QWORD PTR [g_pOrigGL + 2568]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex2i ENDP

glVertex2iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2576]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex2iv ENDP

glVertex2s PROC
    mov rax, QWORD PTR [g_pOrigGL + 2584]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex2s ENDP

glVertex2sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2592]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex2sv ENDP

glVertex3d PROC
    mov rax, QWORD PTR [g_pOrigGL + 2600]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex3d ENDP

glVertex3dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2608]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex3dv ENDP

glVertex3f PROC
    mov rax, QWORD PTR [g_pOrigGL + 2616]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex3f ENDP

glVertex3fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2624]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex3fv ENDP

glVertex3i PROC
    mov rax, QWORD PTR [g_pOrigGL + 2632]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex3i ENDP

glVertex3iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2640]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex3iv ENDP

glVertex3s PROC
    mov rax, QWORD PTR [g_pOrigGL + 2648]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex3s ENDP

glVertex3sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2656]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex3sv ENDP

glVertex4d PROC
    mov rax, QWORD PTR [g_pOrigGL + 2664]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex4d ENDP

glVertex4dv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2672]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex4dv ENDP

glVertex4f PROC
    mov rax, QWORD PTR [g_pOrigGL + 2680]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex4f ENDP

glVertex4fv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2688]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex4fv ENDP

glVertex4i PROC
    mov rax, QWORD PTR [g_pOrigGL + 2696]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex4i ENDP

glVertex4iv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2704]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex4iv ENDP

glVertex4s PROC
    mov rax, QWORD PTR [g_pOrigGL + 2712]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex4s ENDP

glVertex4sv PROC
    mov rax, QWORD PTR [g_pOrigGL + 2720]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertex4sv ENDP

glVertexPointer PROC
    mov rax, QWORD PTR [g_pOrigGL + 2728]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glVertexPointer ENDP

glViewport PROC
    mov rax, QWORD PTR [g_pOrigGL + 2736]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
glViewport ENDP

wglChoosePixelFormat PROC
    mov rax, QWORD PTR [g_pOrigGL + 2744]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglChoosePixelFormat ENDP

wglCopyContext PROC
    mov rax, QWORD PTR [g_pOrigGL + 2752]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglCopyContext ENDP

wglCreateContext PROC
    mov rax, QWORD PTR [g_pOrigGL + 2760]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglCreateContext ENDP

wglCreateLayerContext PROC
    mov rax, QWORD PTR [g_pOrigGL + 2768]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglCreateLayerContext ENDP

wglDeleteContext PROC
    mov rax, QWORD PTR [g_pOrigGL + 2776]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglDeleteContext ENDP

wglDescribeLayerPlane PROC
    mov rax, QWORD PTR [g_pOrigGL + 2784]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglDescribeLayerPlane ENDP

wglDescribePixelFormat PROC
    mov rax, QWORD PTR [g_pOrigGL + 2792]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglDescribePixelFormat ENDP

wglGetCurrentContext PROC
    mov rax, QWORD PTR [g_pOrigGL + 2800]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglGetCurrentContext ENDP

wglGetCurrentDC PROC
    mov rax, QWORD PTR [g_pOrigGL + 2808]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglGetCurrentDC ENDP

wglGetDefaultProcAddress PROC
    mov rax, QWORD PTR [g_pOrigGL + 2816]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglGetDefaultProcAddress ENDP

wglGetLayerPaletteEntries PROC
    mov rax, QWORD PTR [g_pOrigGL + 2824]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglGetLayerPaletteEntries ENDP

wglGetPixelFormat PROC
    mov rax, QWORD PTR [g_pOrigGL + 2832]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglGetPixelFormat ENDP

; wglGetProcAddress (idx 355, byte offset 2840) - overridden by gl_hooks.cpp
;     to return our sys_glBindFramebuffer for glBindFramebuffer/EXT/ARB.
; wglMakeCurrent    (idx 356, byte offset 2848) - overridden by gl_hooks.cpp
;     to lazy-create per-eye FBOs at the window's client size.

wglRealizeLayerPalette PROC
    mov rax, QWORD PTR [g_pOrigGL + 2856]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglRealizeLayerPalette ENDP

wglSetLayerPaletteEntries PROC
    mov rax, QWORD PTR [g_pOrigGL + 2864]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglSetLayerPaletteEntries ENDP

wglSetPixelFormat PROC
    mov rax, QWORD PTR [g_pOrigGL + 2872]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglSetPixelFormat ENDP

wglShareLists PROC
    mov rax, QWORD PTR [g_pOrigGL + 2880]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglShareLists ENDP

; wglSwapBuffers (idx 361, byte offset 2888) - overridden by gl_hooks.cpp
;     to blit the active-eye FBO into the real default framebuffer before swap.

wglSwapLayerBuffers PROC
    mov rax, QWORD PTR [g_pOrigGL + 2896]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglSwapLayerBuffers ENDP

wglSwapMultipleBuffers PROC
    mov rax, QWORD PTR [g_pOrigGL + 2904]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglSwapMultipleBuffers ENDP

wglUseFontBitmapsA PROC
    mov rax, QWORD PTR [g_pOrigGL + 2912]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglUseFontBitmapsA ENDP

wglUseFontBitmapsW PROC
    mov rax, QWORD PTR [g_pOrigGL + 2920]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglUseFontBitmapsW ENDP

wglUseFontOutlinesA PROC
    mov rax, QWORD PTR [g_pOrigGL + 2928]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglUseFontOutlinesA ENDP

wglUseFontOutlinesW PROC
    mov rax, QWORD PTR [g_pOrigGL + 2936]
    test rax, rax
    jz short @F
    jmp rax
@@:
    ret
wglUseFontOutlinesW ENDP

END
