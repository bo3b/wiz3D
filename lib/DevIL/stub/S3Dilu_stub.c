/*
 * S3Dilu stub - minimal no-op implementation of ILU functions
 * for x64. Provides exports so the wrapper can load.
 */
#include <windows.h>

typedef unsigned int ILuint;
typedef unsigned char ILboolean;

#define IL_FALSE 0
#define IL_TRUE  1

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    (void)hinstDLL; (void)fdwReason; (void)lpvReserved;
    return TRUE;
}

ILboolean __stdcall iluFlipImage(void)
{
    return IL_FALSE;
}

void __stdcall iluInit(void)
{
}

ILboolean __stdcall iluScale(ILuint Width, ILuint Height, ILuint Depth)
{
    (void)Width; (void)Height; (void)Depth;
    return IL_FALSE;
}
