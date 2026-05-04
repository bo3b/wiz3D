// NvApiProxy.cpp
// Fake nvapi.dll (x86) / nvapi64.dll (x64)
//
// Wraps NVIDIA's NVAPI surface so that 3D Vision-aware games (e.g.
// Batman: Arkham Asylum) believe they are running on a 3D Vision-capable
// NVIDIA GPU.  Lets such games enable their built-in stereo render path
// on AMD/Intel hardware, which wiz3D's d3d9 wrapper then converts to the
// configured output (anaglyph, SBS, SR weave, etc.).
//
// Two operating modes, auto-detected at first call:
//   PASSTHROUGH: real NVIDIA NVAPI is present in System32 AND succeeds at
//     NvAPI_Initialize.  Forward every call to it untouched.  This keeps
//     us harmless on real NVIDIA systems and avoids breaking anyone's
//     genuine 3D Vision setup.
//   SPOOF: real NVAPI absent or failing.  Answer the game's calls with
//     fake "NVIDIA RTX 2080 Ti running driver 426.06" responses.  Stereo
//     APIs all succeed but no-op since wiz3D handles the actual stereo
//     downstream of D3D.
//
// NVAPI exposes its entire surface through a single export,
//   void* nvapi_QueryInterface(NvU32 functionId);
// which returns __cdecl function pointers indexed by 32-bit IDs.  Function
// IDs and signatures are taken from the MIT-licensed NVAPI 2026 SDK
// headers in lib/nvapi_2026/.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>

// ============================================================
// Local NVAPI types (subset; signatures must match real NVAPI).
// Pulled from lib/nvapi_2026/{nvapi_lite_common.h, nvapi_lite_stereo.h, nvapi.h}
// rather than including the full headers — keeps this DLL self-contained
// and avoids the SAL annotation / version-macro machinery we don't need.
// ============================================================
typedef int                NvAPI_Status;
typedef unsigned int       NvU32;
typedef unsigned char      NvU8;
typedef unsigned __int64   NvU64;
typedef void*              StereoHandle;
typedef void*              NvPhysicalGpuHandle;
typedef void*              NvLogicalGpuHandle;
typedef void*              NvDisplayHandle;

#define NVAPI_OK                     0
#define NVAPI_ERROR                 -1
#define NVAPI_END_ENUMERATION       -7
#define NVAPI_NOT_SUPPORTED       -104

#define NVAPI_SHORT_STRING_MAX     64
#define NVAPI_MAX_PHYSICAL_GPUS    64
typedef char NvAPI_ShortString[NVAPI_SHORT_STRING_MAX];

#define MAKE_NVAPI_VERSION(t, v) ((NvU32)(sizeof(t) | ((v) << 16)))

typedef struct _NV_DISPLAY_DRIVER_VERSION
{
    NvU32              version;
    NvU32              drvVersion;
    NvU32              bldChangeListNum;
    NvAPI_ShortString  szBuildBranchString;
    NvAPI_ShortString  szAdapterString;
} NV_DISPLAY_DRIVER_VERSION;
#define NV_DISPLAY_DRIVER_VERSION_VER  MAKE_NVAPI_VERSION(NV_DISPLAY_DRIVER_VERSION, 1)

typedef enum {
    NV_SYSTEM_TYPE_UNKNOWN = 0,
    NV_SYSTEM_TYPE_LAPTOP  = 1,
    NV_SYSTEM_TYPE_DESKTOP = 2,
} NV_SYSTEM_TYPE;

typedef enum {
    NV_GPU_TYPE_UNKNOWN = 0,
    NV_GPU_TYPE_IGPU    = 1,
    NV_GPU_TYPE_DGPU    = 2,
} NV_GPU_TYPE;

typedef enum {
    NVAPI_STEREO_EYE_RIGHT = 1,
    NVAPI_STEREO_EYE_LEFT  = 2,
    NVAPI_STEREO_EYE_MONO  = 3,
} NV_STEREO_ACTIVE_EYE;

typedef enum {
    NVAPI_STEREO_DRIVER_MODE_AUTOMATIC = 0,
    NVAPI_STEREO_DRIVER_MODE_DIRECT    = 2,
} NV_STEREO_DRIVER_MODE;

typedef enum {
    NVAPI_STEREO_SURFACECREATEMODE_AUTO        = 0,
    NVAPI_STEREO_SURFACECREATEMODE_FORCESTEREO = 1,
    NVAPI_STEREO_SURFACECREATEMODE_FORCEMONO   = 2,
} NVAPI_STEREO_SURFACECREATEMODE;

#define NVAPI_INTERFACE NvAPI_Status __cdecl

// ============================================================
// Diagnostic log (next to the DLL, i.e. in the game folder)
// ============================================================
static HMODULE g_hSelf = nullptr;

static void WriteLog(const char* msg)
{
    wchar_t dir[MAX_PATH] = {};
    if (g_hSelf)
    {
        GetModuleFileNameW(g_hSelf, dir, MAX_PATH);
        wchar_t* p = wcsrchr(dir, L'\\');
        if (p) p[1] = L'\0';
    }
    if (!dir[0] && !GetTempPathW(MAX_PATH, dir)) return;

    wchar_t path[MAX_PATH];
    wcscpy_s(path, dir);
    wcscat_s(path, L"NvApiProxy.log");
    HANDLE h = CreateFileW(path, FILE_APPEND_DATA,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return;
    DWORD written;
    WriteFile(h, msg, static_cast<DWORD>(strlen(msg)), &written, nullptr);
    CloseHandle(h);
}

// ============================================================
// Spoofed identity
//   RTX 2080 Ti — last NVIDIA card to ship with official 3D Vision support.
//   Driver 426.06 — last 3D Vision-aware NVIDIA driver (beta).  Some games
//   bail on driver versions below ~418, and 426.06 satisfies all known
//   3D Vision title minimums while still being recognisably "of the era".
// ============================================================
static const char* kSpoofGpuName       = "NVIDIA GeForce RTX 2080 Ti";
static const char* kSpoofBranchStr     = "r426_00-100";
static const NvU32 kSpoofDrvVersion    = 42606;       // major*100 + minor (NVAPI convention)
static const NvU32 kSpoofBldChangeNum  = 25739267;
static const NvU32 kSpoofPciVendor     = 0x10DE;       // NVIDIA
static const NvU32 kSpoofPciDevice     = 0x1E07;       // RTX 2080 Ti (TU102)
static const NvU32 kSpoofPciSubVendor  = 0x10DE;
static const NvU32 kSpoofPciSubDevice  = 0x12FF;
static const NvU32 kSpoofPciRevision   = 0xA1;

static const NvPhysicalGpuHandle kFakePhysicalGpu = (NvPhysicalGpuHandle)0xFA1E0001ULL;
static const NvLogicalGpuHandle  kFakeLogicalGpu  = (NvLogicalGpuHandle) 0xFA1E1001ULL;
static const NvDisplayHandle     kFakeDisplay     = (NvDisplayHandle)    0xFA1ED15AULL;
typedef void* NvUnAttachedDisplayHandle;

// ============================================================
// Mode detection: passthrough to real NVIDIA NVAPI, or spoof?
// Lazy-initialised on first nvapi_QueryInterface call (DllMain is too
// early to LoadLibrary safely).
// ============================================================
typedef void* (__cdecl *PFN_nvapi_QueryInterface)(NvU32 id);

static bool                     g_bInitialised   = false;
static bool                     g_bPassthrough   = false;
static HMODULE                  g_hRealNvapi     = nullptr;
static PFN_nvapi_QueryInterface g_pfnRealQI      = nullptr;

static void InitMode()
{
    if (g_bInitialised) return;
    g_bInitialised = true;

    wchar_t sysDir[MAX_PATH] = {};
    GetSystemDirectoryW(sysDir, MAX_PATH);

    wchar_t path[MAX_PATH];
    wcscpy_s(path, sysDir);
#ifdef _WIN64
    wcscat_s(path, L"\\nvapi64.dll");
#else
    // 32-bit processes get File System Redirection: "System32\nvapi.dll"
    // resolves to SysWOW64\nvapi.dll automatically.
    wcscat_s(path, L"\\nvapi.dll");
#endif

    g_hRealNvapi = LoadLibraryW(path);
    if (!g_hRealNvapi)
    {
        WriteLog("[NvApiProxy] no real NVAPI on system -> SPOOF mode\n");
        return;
    }

    g_pfnRealQI = (PFN_nvapi_QueryInterface)
        GetProcAddress(g_hRealNvapi, "nvapi_QueryInterface");
    if (!g_pfnRealQI)
    {
        FreeLibrary(g_hRealNvapi);
        g_hRealNvapi = nullptr;
        WriteLog("[NvApiProxy] real NVAPI loaded but no QueryInterface -> SPOOF mode\n");
        return;
    }

    // Verify the real NVAPI actually works (handles orphaned nvapi.dll left
    // behind by an uninstalled NVIDIA driver, or a non-functional driver).
    typedef NvAPI_Status (__cdecl *PFN_Initialize)(void);
    PFN_Initialize pfnInit = (PFN_Initialize)g_pfnRealQI(0x0150E828);
    if (!pfnInit || pfnInit() != NVAPI_OK)
    {
        FreeLibrary(g_hRealNvapi);
        g_hRealNvapi = nullptr;
        g_pfnRealQI  = nullptr;
        WriteLog("[NvApiProxy] real NvAPI_Initialize failed -> SPOOF mode\n");
        return;
    }

    g_bPassthrough = true;
    WriteLog("[NvApiProxy] real NVIDIA driver detected -> PASSTHROUGH mode\n");
}

// ============================================================
// Spoofed implementations
// ============================================================
//
// Per-handle state isn't tracked; one global block suffices because wiz3D
// performs the actual stereoization downstream regardless of what the game
// asks for here.  If a real-world game proves to need per-device isolation
// (separate IPD/convergence per swapchain), promote this to a small map.
struct FakeStereoState
{
    NvU8                            isActive;
    float                           separation;     // 0..100 percent
    float                           convergence;
    NV_STEREO_ACTIVE_EYE            activeEye;
    NV_STEREO_DRIVER_MODE           driverMode;
    NVAPI_STEREO_SURFACECREATEMODE  createMode;
    NvU32                           frustumAdjust;
};

static FakeStereoState g_Stereo = {
    /*isActive*/      1,
    /*separation*/    15.0f,
    /*convergence*/   0.5f,
    /*activeEye*/     NVAPI_STEREO_EYE_LEFT,
    /*driverMode*/    NVAPI_STEREO_DRIVER_MODE_AUTOMATIC,
    /*createMode*/    NVAPI_STEREO_SURFACECREATEMODE_AUTO,
    /*frustumAdjust*/ 1,
};

// --- general / driver -------------------------------------------------------
NVAPI_INTERFACE Spoof_Initialize(void) { return NVAPI_OK; }
NVAPI_INTERFACE Spoof_Unload(void)     { return NVAPI_OK; }

NVAPI_INTERFACE Spoof_GetErrorMessage(NvAPI_Status nr, NvAPI_ShortString szDesc)
{
    if (!szDesc) return NVAPI_ERROR;
    _snprintf_s(szDesc, NVAPI_SHORT_STRING_MAX, _TRUNCATE,
                "NvApiProxy: status %d", (int)nr);
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_GetInterfaceVersionString(NvAPI_ShortString szDesc)
{
    if (!szDesc) return NVAPI_ERROR;
    strcpy_s(szDesc, NVAPI_SHORT_STRING_MAX, kSpoofBranchStr);
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_SYS_GetDriverAndBranchVersion(NvU32* pDrv, NvAPI_ShortString szBranch)
{
    if (!pDrv || !szBranch) return NVAPI_ERROR;
    *pDrv = kSpoofDrvVersion;
    strcpy_s(szBranch, NVAPI_SHORT_STRING_MAX, kSpoofBranchStr);
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_GetDisplayDriverVersion(NvDisplayHandle, NV_DISPLAY_DRIVER_VERSION* pVer)
{
    if (!pVer) return NVAPI_ERROR;
    pVer->version          = NV_DISPLAY_DRIVER_VERSION_VER;
    pVer->drvVersion       = kSpoofDrvVersion;
    pVer->bldChangeListNum = kSpoofBldChangeNum;
    strcpy_s(pVer->szBuildBranchString, NVAPI_SHORT_STRING_MAX, kSpoofBranchStr);
    strcpy_s(pVer->szAdapterString,     NVAPI_SHORT_STRING_MAX, kSpoofGpuName);
    return NVAPI_OK;
}

// --- GPU enumeration --------------------------------------------------------
NVAPI_INTERFACE Spoof_EnumPhysicalGPUs(NvPhysicalGpuHandle h[NVAPI_MAX_PHYSICAL_GPUS], NvU32* pCount)
{
    if (!h || !pCount) return NVAPI_ERROR;
    h[0] = kFakePhysicalGpu;
    for (int i = 1; i < NVAPI_MAX_PHYSICAL_GPUS; ++i) h[i] = nullptr;
    *pCount = 1;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_EnumLogicalGPUs(NvLogicalGpuHandle h[NVAPI_MAX_PHYSICAL_GPUS], NvU32* pCount)
{
    if (!h || !pCount) return NVAPI_ERROR;
    h[0] = kFakeLogicalGpu;
    for (int i = 1; i < NVAPI_MAX_PHYSICAL_GPUS; ++i) h[i] = nullptr;
    *pCount = 1;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_GetPhysicalGPUsFromDisplay(NvDisplayHandle, NvPhysicalGpuHandle h[NVAPI_MAX_PHYSICAL_GPUS], NvU32* pCount)
{
    return Spoof_EnumPhysicalGPUs(h, pCount);
}

// --- display enumeration (3D Vision games iterate displays before stereo setup)
NVAPI_INTERFACE Spoof_EnumNvidiaDisplayHandle(NvU32 thisEnum, NvDisplayHandle* pHandle)
{
    if (!pHandle) return NVAPI_ERROR;
    if (thisEnum == 0)
    {
        *pHandle = kFakeDisplay;
        return NVAPI_OK;
    }
    return NVAPI_END_ENUMERATION;
}

NVAPI_INTERFACE Spoof_EnumNvidiaUnAttachedDisplayHandle(NvU32, NvUnAttachedDisplayHandle* pHandle)
{
    // No unattached displays — only the one primary display.
    if (pHandle) *pHandle = nullptr;
    return NVAPI_END_ENUMERATION;
}

NVAPI_INTERFACE Spoof_GetAssociatedNvidiaDisplayHandle(const char* /*szDisplayName*/, NvDisplayHandle* pHandle)
{
    if (!pHandle) return NVAPI_ERROR;
    *pHandle = kFakeDisplay;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_GetAssociatedDisplayOutputId(NvDisplayHandle, NvU32* pOutputId)
{
    if (!pOutputId) return NVAPI_ERROR;
    *pOutputId = 0x00000001;  // arbitrary non-zero output ID
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_GPU_GetFullName(NvPhysicalGpuHandle, NvAPI_ShortString szName)
{
    if (!szName) return NVAPI_ERROR;
    strcpy_s(szName, NVAPI_SHORT_STRING_MAX, kSpoofGpuName);
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_GPU_GetSystemType(NvPhysicalGpuHandle, NV_SYSTEM_TYPE* p)
{
    if (!p) return NVAPI_ERROR;
    *p = NV_SYSTEM_TYPE_DESKTOP;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_GPU_GetGPUType(NvPhysicalGpuHandle, NV_GPU_TYPE* p)
{
    if (!p) return NVAPI_ERROR;
    *p = NV_GPU_TYPE_DGPU;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_GPU_GetPCIIdentifiers(NvPhysicalGpuHandle,
    NvU32* pDeviceId, NvU32* pSubsystemId, NvU32* pRevisionId, NvU32* pExtDeviceId)
{
    if (pDeviceId)    *pDeviceId    = (kSpoofPciDevice    << 16) | kSpoofPciVendor;
    if (pSubsystemId) *pSubsystemId = (kSpoofPciSubDevice << 16) | kSpoofPciSubVendor;
    if (pRevisionId)  *pRevisionId  = kSpoofPciRevision;
    if (pExtDeviceId) *pExtDeviceId = kSpoofPciDevice;
    return NVAPI_OK;
}

// --- stereo -----------------------------------------------------------------
NVAPI_INTERFACE Spoof_Stereo_Enable(void)  { return NVAPI_OK; }
NVAPI_INTERFACE Spoof_Stereo_Disable(void) { return NVAPI_OK; }

NVAPI_INTERFACE Spoof_Stereo_IsEnabled(NvU8* p)
{
    if (!p) return NVAPI_ERROR;
    *p = 1;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_Stereo_GetStereoSupport(void* /*opaque pCaps*/) { return NVAPI_OK; }

NVAPI_INTERFACE Spoof_Stereo_CreateHandleFromIUnknown(void* pDevice, StereoHandle* pH)
{
    if (!pH) return NVAPI_ERROR;
    // Use the device pointer itself as the handle so it's distinct per device.
    *pH = pDevice ? (StereoHandle)pDevice : (StereoHandle)(uintptr_t)0xFA1ED1CEULL;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_Stereo_DestroyHandle(StereoHandle) { return NVAPI_OK; }

NVAPI_INTERFACE Spoof_Stereo_Activate(StereoHandle)   { g_Stereo.isActive = 1; return NVAPI_OK; }
NVAPI_INTERFACE Spoof_Stereo_Deactivate(StereoHandle) { g_Stereo.isActive = 0; return NVAPI_OK; }
NVAPI_INTERFACE Spoof_Stereo_IsActivated(StereoHandle, NvU8* p)
{
    if (!p) return NVAPI_ERROR;
    *p = g_Stereo.isActive;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_Stereo_GetSeparation(StereoHandle, float* p)
{
    if (!p) return NVAPI_ERROR;
    *p = g_Stereo.separation;
    return NVAPI_OK;
}
NVAPI_INTERFACE Spoof_Stereo_SetSeparation(StereoHandle, float v)
{
    if (v < 0.0f || v > 100.0f) return NVAPI_ERROR;
    g_Stereo.separation = v;
    return NVAPI_OK;
}
NVAPI_INTERFACE Spoof_Stereo_DecreaseSeparation(StereoHandle)
{
    g_Stereo.separation = (g_Stereo.separation > 5.0f) ? g_Stereo.separation - 5.0f : 0.0f;
    return NVAPI_OK;
}
NVAPI_INTERFACE Spoof_Stereo_IncreaseSeparation(StereoHandle)
{
    g_Stereo.separation = (g_Stereo.separation < 95.0f) ? g_Stereo.separation + 5.0f : 100.0f;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_Stereo_GetConvergence(StereoHandle, float* p)
{
    if (!p) return NVAPI_ERROR;
    *p = g_Stereo.convergence;
    return NVAPI_OK;
}
NVAPI_INTERFACE Spoof_Stereo_SetConvergence(StereoHandle, float v)
{
    g_Stereo.convergence = v;
    return NVAPI_OK;
}
NVAPI_INTERFACE Spoof_Stereo_DecreaseConvergence(StereoHandle) { g_Stereo.convergence -= 0.05f; return NVAPI_OK; }
NVAPI_INTERFACE Spoof_Stereo_IncreaseConvergence(StereoHandle) { g_Stereo.convergence += 0.05f; return NVAPI_OK; }

NVAPI_INTERFACE Spoof_Stereo_GetEyeSeparation(StereoHandle, float* p)
{
    if (!p) return NVAPI_ERROR;
    // Ratio of <eye distance>/<screen width>.  ~64mm IPD over ~1m screen.
    *p = 0.064f;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_Stereo_SetActiveEye(StereoHandle, NV_STEREO_ACTIVE_EYE eye)
{
    g_Stereo.activeEye = eye;
    return NVAPI_OK;
}
NVAPI_INTERFACE Spoof_Stereo_SetDriverMode(NV_STEREO_DRIVER_MODE mode)
{
    g_Stereo.driverMode = mode;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_Stereo_IsWindowedModeSupported(NvU8* p)
{
    if (!p) return NVAPI_ERROR;
    *p = 1;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_Stereo_SetSurfaceCreationMode(StereoHandle, NVAPI_STEREO_SURFACECREATEMODE m)
{
    g_Stereo.createMode = m;
    return NVAPI_OK;
}
NVAPI_INTERFACE Spoof_Stereo_GetSurfaceCreationMode(StereoHandle, NVAPI_STEREO_SURFACECREATEMODE* p)
{
    if (!p) return NVAPI_ERROR;
    *p = g_Stereo.createMode;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_Stereo_GetFrustumAdjustMode(StereoHandle, NvU32* p)
{
    if (!p) return NVAPI_ERROR;
    *p = g_Stereo.frustumAdjust;
    return NVAPI_OK;
}
NVAPI_INTERFACE Spoof_Stereo_SetFrustumAdjustMode(StereoHandle, NvU32 mode)
{
    g_Stereo.frustumAdjust = mode;
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_Stereo_InitActivation(StereoHandle, NvU8 /*enable*/)        { return NVAPI_OK; }
NVAPI_INTERFACE Spoof_Stereo_Trigger_Activation(StereoHandle)                     { return NVAPI_OK; }
NVAPI_INTERFACE Spoof_Stereo_ReverseStereoBlitControl(StereoHandle, NvU8 /*on*/)  { return NVAPI_OK; }
NVAPI_INTERFACE Spoof_Stereo_SetNotificationMessage(StereoHandle, NvU64, NvU64)   { return NVAPI_OK; }

NVAPI_INTERFACE Spoof_Stereo_SetDefaultProfile(const char* /*szName*/)            { return NVAPI_OK; }
NVAPI_INTERFACE Spoof_Stereo_GetDefaultProfile(NvU32 /*cbIn*/, char* /*szName*/, NvU32* pcbOut)
{
    if (!pcbOut) return NVAPI_ERROR;
    *pcbOut = 0;  // no default profile set
    return NVAPI_OK;
}

NVAPI_INTERFACE Spoof_Stereo_Debug_WasLastDrawStereoized(StereoHandle, NvU8* p)
{
    if (!p) return NVAPI_ERROR;
    *p = 1;
    return NVAPI_OK;
}

// Catch-all for IDs we haven't explicitly mapped.  Returning NVAPI_NOT_SUPPORTED
// causes games to gracefully skip optional features (DLSS, Reflex, telemetry,
// etc.) without aborting.
NVAPI_INTERFACE Spoof_NotSupported(void) { return NVAPI_NOT_SUPPORTED; }

// Placeholder for known-called-but-not-yet-identified function IDs.  Returning
// NVAPI_OK (with no output written) is more permissive than NOT_SUPPORTED:
// 3D Vision-era games tend to interpret NOT_SUPPORTED as "this entire NVAPI is
// missing capability X, give up", while OK is treated as "optional check
// passed".  Used for IDs we've observed in the wild but can't yet name.
NVAPI_INTERFACE Spoof_OkNoOp(void) { return NVAPI_OK; }

// ============================================================
// Dispatcher — the single export every NVAPI consumer goes through.
// IDs from lib/nvapi_2026/nvapi_interface.h.
// ============================================================
extern "C" __declspec(dllexport) void* __cdecl nvapi_QueryInterface(NvU32 id)
{
    InitMode();

    if (g_bPassthrough)
        return g_pfnRealQI(id);

    switch (id)
    {
        // sys / general
        case 0x0150E828: return (void*)&Spoof_Initialize;
        case 0xD22BDD7E: return (void*)&Spoof_Unload;
        case 0x6C2D048C: return (void*)&Spoof_GetErrorMessage;
        case 0x01053FA5: return (void*)&Spoof_GetInterfaceVersionString;

        // driver / GPU
        case 0x2926AAAD: return (void*)&Spoof_SYS_GetDriverAndBranchVersion;
        case 0xF951A4D1: return (void*)&Spoof_GetDisplayDriverVersion;
        case 0xE5AC921F: return (void*)&Spoof_EnumPhysicalGPUs;
        case 0x48B3EA59: return (void*)&Spoof_EnumLogicalGPUs;
        case 0x34EF9506: return (void*)&Spoof_GetPhysicalGPUsFromDisplay;
        case 0x9ABDD40D: return (void*)&Spoof_EnumNvidiaDisplayHandle;
        case 0x20DE9260: return (void*)&Spoof_EnumNvidiaUnAttachedDisplayHandle;
        case 0x35C29134: return (void*)&Spoof_GetAssociatedNvidiaDisplayHandle;
        case 0xD995937E: return (void*)&Spoof_GetAssociatedDisplayOutputId;

        // Observed-in-the-wild IDs not in the public 2026 SDK header (likely
        // private NVAPI functions that 3D Vision-era games still call).  Return
        // NVAPI_OK so the game proceeds rather than bailing on NOT_SUPPORTED.
        // If a game ends up needing real output values from these, we'll need
        // to identify the actual function and implement it.
        case 0x33C7358C: return (void*)&Spoof_OkNoOp;
        case 0x36E39E6B: return (void*)&Spoof_OkNoOp;

        case 0xCEEE8E9F: return (void*)&Spoof_GPU_GetFullName;
        case 0xBAAABFCC: return (void*)&Spoof_GPU_GetSystemType;
        case 0xC33BAEB1: return (void*)&Spoof_GPU_GetGPUType;
        case 0x2DDFB66E: return (void*)&Spoof_GPU_GetPCIIdentifiers;

        // stereo
        case 0x239C4545: return (void*)&Spoof_Stereo_Enable;
        case 0x2EC50C2B: return (void*)&Spoof_Stereo_Disable;
        case 0x348FF8E1: return (void*)&Spoof_Stereo_IsEnabled;
        case 0x296C434D: return (void*)&Spoof_Stereo_GetStereoSupport;
        case 0xAC7E37F4: return (void*)&Spoof_Stereo_CreateHandleFromIUnknown;
        case 0x3A153134: return (void*)&Spoof_Stereo_DestroyHandle;
        case 0xF6A1AD68: return (void*)&Spoof_Stereo_Activate;
        case 0x2D68DE96: return (void*)&Spoof_Stereo_Deactivate;
        case 0x1FB0BC30: return (void*)&Spoof_Stereo_IsActivated;
        case 0x451F2134: return (void*)&Spoof_Stereo_GetSeparation;
        case 0x5C069FA3: return (void*)&Spoof_Stereo_SetSeparation;
        case 0xDA044458: return (void*)&Spoof_Stereo_DecreaseSeparation;
        case 0xC9A8ECEC: return (void*)&Spoof_Stereo_IncreaseSeparation;
        case 0x4AB00934: return (void*)&Spoof_Stereo_GetConvergence;
        case 0x3DD6B54B: return (void*)&Spoof_Stereo_SetConvergence;
        case 0x4C87E317: return (void*)&Spoof_Stereo_DecreaseConvergence;
        case 0xA17DAABE: return (void*)&Spoof_Stereo_IncreaseConvergence;
        case 0xE6839B43: return (void*)&Spoof_Stereo_GetFrustumAdjustMode;
        case 0x7BE27FA2: return (void*)&Spoof_Stereo_SetFrustumAdjustMode;
        case 0xC7177702: return (void*)&Spoof_Stereo_InitActivation;
        case 0x0D6C6CD2: return (void*)&Spoof_Stereo_Trigger_Activation;
        case 0x3CD58F89: return (void*)&Spoof_Stereo_ReverseStereoBlitControl;
        case 0x6B9B409E: return (void*)&Spoof_Stereo_SetNotificationMessage;
        case 0x96EEA9F8: return (void*)&Spoof_Stereo_SetActiveEye;
        case 0x5E8F0BEC: return (void*)&Spoof_Stereo_SetDriverMode;
        case 0xCE653127: return (void*)&Spoof_Stereo_GetEyeSeparation;
        case 0x40C8ED5E: return (void*)&Spoof_Stereo_IsWindowedModeSupported;
        case 0xF5DCFCBA: return (void*)&Spoof_Stereo_SetSurfaceCreationMode;
        case 0x36F1C736: return (void*)&Spoof_Stereo_GetSurfaceCreationMode;
        case 0xED4416C5: return (void*)&Spoof_Stereo_Debug_WasLastDrawStereoized;
        case 0x44F0ECD1: return (void*)&Spoof_Stereo_SetDefaultProfile;
        case 0x624E21C2: return (void*)&Spoof_Stereo_GetDefaultProfile;

        default:
        {
            // Empirically (Batman: Arkham Asylum on AMD, full log), 3D Vision-era
            // games query 200+ NVAPI functions during startup — most undocumented
            // and not in the public 2026 SDK. Returning NOT_SUPPORTED for all
            // unknowns made games bail on the first introspection call.
            //
            // OK-no-op is more permissive: game treats the call as "succeeded,
            // returned no info" rather than "this NVAPI is broken". The risk is
            // OUT params remain whatever the caller initialised them to (often
            // zero), which can cause subtle issues if the game depends on
            // populated outputs. Tradeoff worth taking — better to ship and
            // identify specific blockers than ship-block on every unknown.
            char buf[80];
            _snprintf_s(buf, sizeof(buf), _TRUNCATE,
                        "[NvApiProxy] unknown id=0x%08X -> OK (default)\n", id);
            WriteLog(buf);
            return (void*)&Spoof_OkNoOp;
        }
    }
}

// Some games look up the uppercase symbol; route to the same dispatcher.
extern "C" __declspec(dllexport) void* __cdecl NvAPI_QueryInterface(NvU32 id)
{
    return nvapi_QueryInterface(id);
}

// ============================================================
// DllMain
// ============================================================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        g_hSelf = hModule;
        DisableThreadLibraryCalls(hModule);
        WriteLog("[NvApiProxy] DLL_PROCESS_ATTACH\n");
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        if (g_hRealNvapi)
        {
            FreeLibrary(g_hRealNvapi);
            g_hRealNvapi = nullptr;
            g_pfnRealQI  = nullptr;
        }
    }
    return TRUE;
}
