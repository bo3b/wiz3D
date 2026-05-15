#include "stdafx.h"
#include "Wiz3DStereoApi.h"
#include "BaseStereoRenderer.h"
#include "..\S3DAPI\GlobalData.h"
#include "..\S3DAPI\ReadData.h"   // WriteInputData for UserProfile.xml persistence

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
	WriteInputData(&g_pActiveRenderer->m_Input);
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
	WriteInputData(&g_pActiveRenderer->m_Input);
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
	WriteInputData(&g_pActiveRenderer->m_Input);
}

void Wiz3D_StepSeparation(int dir)
{
	if (!g_pActiveRenderer)
		return;
	CameraPreset* p = g_pActiveRenderer->m_Input.GetActivePreset();
	float step = (dir > 0) ? STEP_STEREOBASE : -STEP_STEREOBASE;
	p->StereoBase += step;
	if (p->StereoBase < 0.0f) p->StereoBase = 0.0f;
	if (p->StereoBase > MAX_STEREOBASE) p->StereoBase = MAX_STEREOBASE;
	WriteInputData(&g_pActiveRenderer->m_Input);
}

void Wiz3D_StepConvergence(int dir)
{
	// "Increase convergence" pushes parallax plane FURTHER away (Z_conv up).
	// Since One_div_ZPS = 1/Z_conv, that maps to One_div_ZPS DECREASING.
	if (!g_pActiveRenderer)
		return;
	CameraPreset* p = g_pActiveRenderer->m_Input.GetActivePreset();
	float step = (dir > 0) ? -STEP_ONE_DIV_ZPS : STEP_ONE_DIV_ZPS;
	p->One_div_ZPS += step;
	if (p->One_div_ZPS < MIN_ONE_DIV_ZPS) p->One_div_ZPS = MIN_ONE_DIV_ZPS;
	if (p->One_div_ZPS > MAX_ONE_DIV_ZPS) p->One_div_ZPS = MAX_ONE_DIV_ZPS;
	WriteInputData(&g_pActiveRenderer->m_Input);
}

int Wiz3D_HasProfileEntry()
{
	// gInfo flags are set during ReadProfilesAllParts when the running game's
	// exe matches an entry under <Profile><File Name="..."> in any of the
	// three profile files. No renderer required — these reflect static load
	// state from process startup, even if the renderer hasn't registered yet.
	return (gInfo.bMatchedInBase || gInfo.bMatchedInCommunity || gInfo.bMatchedInUser) ? 1 : 0;
}

} // extern "C"
