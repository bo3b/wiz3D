/*
 * S3DDevIL stub - minimal no-op implementation of DevIL functions
 * used by iZ3D wrapper DLLs. Provides the exports so the wrapper
 * can load, but image operations are no-ops (screenshots won't work).
 *
 * Replace with real DevIL 1.7.8 DLL for full screenshot support.
 */

#include <windows.h>

typedef unsigned int ILuint;
typedef int          ILint;
typedef int          ILsizei;
typedef unsigned char ILubyte;
typedef unsigned char ILboolean;
typedef unsigned int ILenum;

#define IL_FALSE 0
#define IL_TRUE  1

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    (void)hinstDLL; (void)fdwReason; (void)lpvReserved;
    return TRUE;
}

/*
 * Functions use __stdcall to match the original DevIL calling convention.
 * No __declspec(dllexport) — exports are controlled by linker options.
 */

void __stdcall ilInit(void)
{
    /* no-op */
}

void __stdcall ilBindImage(ILuint Image)
{
    (void)Image;
}

ILboolean __stdcall ilConvertImage(ILenum DestFormat, ILenum DestType)
{
    (void)DestFormat; (void)DestType;
    return IL_FALSE;
}

void __stdcall ilDeleteImage(ILuint Num)
{
    (void)Num;
}

void __stdcall ilDeleteImages(ILsizei Num, const ILuint *Images)
{
    (void)Num; (void)Images;
}

ILuint __stdcall ilGenImage(void)
{
    return 1;
}

void __stdcall ilGenImages(ILsizei Num, ILuint *Images)
{
    ILsizei i;
    if (Images) {
        for (i = 0; i < Num; i++)
            Images[i] = (ILuint)(i + 1);
    }
}

ILubyte* __stdcall ilGetData(void)
{
    return NULL;
}

ILint __stdcall ilGetInteger(ILenum Mode)
{
    (void)Mode;
    return 0;
}

ILboolean __stdcall ilLoadImage(const wchar_t *FileName)
{
    (void)FileName;
    return IL_FALSE;
}

ILboolean __stdcall ilLoadL(ILenum Type, const void *Lump, ILuint Size)
{
    (void)Type; (void)Lump; (void)Size;
    return IL_FALSE;
}

ILboolean __stdcall ilSave(ILenum Type, const wchar_t *FileName)
{
    (void)Type; (void)FileName;
    return IL_FALSE;
}

ILboolean __stdcall ilTexImage(ILuint Width, ILuint Height, ILuint Depth,
    ILubyte Bpp, ILenum Format, ILenum Type, void *Data)
{
    (void)Width; (void)Height; (void)Depth;
    (void)Bpp; (void)Format; (void)Type; (void)Data;
    return IL_FALSE;
}
