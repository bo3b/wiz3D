#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>

static wchar_t g_dxerr_buf[512];
static char g_dxerr_bufA[512];

const wchar_t* __stdcall DXGetErrorStringW(long hr) {
    swprintf_s(g_dxerr_buf, 512, L"HRESULT 0x%08lX", (unsigned long)hr);
    return g_dxerr_buf;
}

const char* __stdcall DXGetErrorStringA(long hr) {
    sprintf_s(g_dxerr_bufA, 512, "HRESULT 0x%08lX", (unsigned long)hr);
    return g_dxerr_bufA;
}

const wchar_t* __stdcall DXGetErrorDescriptionW(long hr) {
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, (DWORD)hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        g_dxerr_buf, 512, NULL);
    return g_dxerr_buf;
}

const char* __stdcall DXGetErrorDescriptionA(long hr) {
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, (DWORD)hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        g_dxerr_bufA, 512, NULL);
    return g_dxerr_bufA;
}

long __stdcall DXTraceW(const char* file, unsigned long line, long hr, const wchar_t* msg, int bPopup) {
    (void)file; (void)line; (void)msg; (void)bPopup;
    return hr;
}

long __stdcall DXTraceA(const char* file, unsigned long line, long hr, const char* msg, int bPopup) {
    (void)file; (void)line; (void)msg; (void)bPopup;
    return hr;
}
