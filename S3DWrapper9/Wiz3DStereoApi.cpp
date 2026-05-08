#include "stdafx.h"
#include "Wiz3DStereoApi.h"
#include "BaseStereoRenderer.h"
#include "..\S3DAPI\GlobalData.h"

// Single active renderer. wiz3D's typical case is one IDirect3DDevice9 per
// process, so a single global pointer suffices. If a process ever creates
// multiple devices, the most-recently-registered one wins - that's the
// surface games expect from NvAPI's stereo handle anyway, since NvAPI itself
// gates everything through one-handle-per-device-but-one-active-stereo-state.
//
// Mutation happens from the game thread (via wrapper Reset / OSD hotkey
// changes) and from NvAPI threads. Float reads/writes are atomic on x86/x64
// for naturally-aligned values; we keep the shared state to a single pointer
// + getters/setters that touch one float at a time, no struct copying.
static CBaseStereoRenderer* g_pActiveRenderer = nullptr;

extern "C"
{

void Wiz3D_RegisterActiveRenderer(CBaseStereoRenderer* pRenderer)
{
	g_pActiveRenderer = pRenderer;
}

void Wiz3D_UnregisterActiveRenderer(CBaseStereoRenderer* pRenderer)
{
	if (g_pActiveRenderer == pRenderer)
		g_pActiveRenderer = nullptr;
}

// When no renderer is registered yet (NvAPI-aware code queries us before
// CreateDevice — HelixMod and 3D Vision-aware games typically do this at
// process startup), return *sensible non-zero defaults* rather than 0/false.
// Returning zeros caused HelixMod to immediately destroy its stereo handle
// and disable itself ("stereo isn't usable, never mind") - confirmed via
// Valkyria Chronicles trace 2026-05-08. Defaults chosen so:
//   - IsActivated = 1   : caller proceeds with stereo setup
//   - Separation  = 50% : neutral mid-slider value
//   - Convergence = 1.0 : 1 world-unit depth
// Once CreateDevice runs and the renderer registers, subsequent queries
// switch to real wiz3D values.

int Wiz3D_GetStereoActive()
{
	if (!g_pActiveRenderer)
		return 1;
	return g_pActiveRenderer->m_Input.StereoActive ? 1 : 0;
}

void Wiz3D_SetStereoActive(int active)
{
	if (!g_pActiveRenderer)
		return; // no-op; caller will retry once renderer registers
	g_pActiveRenderer->m_Input.StereoActive = active ? true : false;
}

float Wiz3D_GetSeparationPercent()
{
	if (!g_pActiveRenderer)
		return 50.0f;
	const CameraPreset* p = g_pActiveRenderer->m_Input.GetActivePreset();
	return SEPARATION_TO_PERCENT(p->StereoBase);
}

void Wiz3D_SetSeparationPercent(float percent)
{
	if (!g_pActiveRenderer)
		return;
	if (percent < 0.0f) percent = 0.0f;
	if (percent > 100.0f) percent = 100.0f;
	CameraPreset* p = g_pActiveRenderer->m_Input.GetActivePreset();
	p->StereoBase = PERCENT_TO_SEPARATION(percent);
}

float Wiz3D_GetConvergence()
{
	if (!g_pActiveRenderer)
		return 1.0f;
	const CameraPreset* p = g_pActiveRenderer->m_Input.GetActivePreset();
	if (p->One_div_ZPS == 0.0f)
		return 1.0f;
	return 1.0f / p->One_div_ZPS;
}

void Wiz3D_SetConvergence(float depth)
{
	if (!g_pActiveRenderer)
		return;
	if (depth <= 0.0f)
		return;
	CameraPreset* p = g_pActiveRenderer->m_Input.GetActivePreset();
	p->One_div_ZPS = 1.0f / depth;
}

} // extern "C"
