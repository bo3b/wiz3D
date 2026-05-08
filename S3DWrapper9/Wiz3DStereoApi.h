// wiz3D stereo bridge - exposed exports of S3DWrapperD3D9.dll for other
// in-process modules (specifically NvApiProxy) to read and write the active
// renderer's stereo state.
//
// All getters return wiz3D's authoritative values; all setters update them
// so that calls coming via NvAPI (3D Vision-aware games, HelixMod) are kept
// in sync with the wiz3D config / OSD slider.
//
// Resolved at runtime by NvApiProxy via GetModuleHandle("S3DWrapperD3D9.dll")
// + GetProcAddress. When the wrapper isn't loaded NvApiProxy falls back to
// its own local FakeStereoState so the proxy works standalone.

#pragma once

extern "C"
{
	// Lifetime: called by CBaseStereoRenderer ctor / dtor so the bridge knows
	// which renderer to read state from. Internal to S3DWrapperD3D9; not for
	// external callers.
	void Wiz3D_RegisterActiveRenderer(class CBaseStereoRenderer* pRenderer);
	void Wiz3D_UnregisterActiveRenderer(class CBaseStereoRenderer* pRenderer);

	// Exports for NvApiProxy. All return 0 / false if no renderer is active.
	__declspec(dllexport) int   __cdecl Wiz3D_GetStereoActive();
	__declspec(dllexport) void  __cdecl Wiz3D_SetStereoActive(int active);

	// Separation expressed as a percent (0..100) to match NvAPI's convention.
	__declspec(dllexport) float __cdecl Wiz3D_GetSeparationPercent();
	__declspec(dllexport) void  __cdecl Wiz3D_SetSeparationPercent(float percent);

	// Convergence in world-depth units (matches NvAPI semantics: depth at
	// which both eyes converge, i.e. objects appear at the screen plane).
	// Internally wiz3D stores 1/depth (One_div_ZPS); the bridge inverts.
	__declspec(dllexport) float __cdecl Wiz3D_GetConvergence();
	__declspec(dllexport) void  __cdecl Wiz3D_SetConvergence(float depth);
}
