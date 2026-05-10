/* NvDirectMode/dxgi - layout-stable IDXGIFactory vtable hot-patch (task #70)
 *
 * Replaces the DXGIFactoryProxy class wrap for the swap-chain creation
 * methods. Same pattern as adapter_vtable_hooks.cpp (task #66) and
 * d3d9/device_vtable_hooks.cpp (task #68): hot-patch the real vtable so
 * games that walk struct internals past the vtable (Hitman Absolution,
 * Dirt Rally / Dirt 3 / Dirt Showdown / Grid Autosport, all confirmed
 * Direct Mode) get a layout-stable factory pointer with our hooks still
 * intercepting the calls we care about.
 *
 * Slots patched (stable across Windows versions per the IDL):
 *   IDXGIFactory ::CreateSwapChain                = 10
 *   IDXGIFactory2::CreateSwapChainForHwnd          = 15
 *   IDXGIFactory2::CreateSwapChainForCoreWindow    = 16
 *   IDXGIFactory2::CreateSwapChainForComposition   = 24
 *
 * Vtable is process-shared across every IDXGIFactory* in dxgi.dll, so
 * one patch covers all factories the game creates — including ones we
 * never see (e.g. those returned from IDXGIAdapter::GetParent which
 * previously needed a cross-DLL wrap call).
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dxgi.h>
#include <dxgi1_2.h>

namespace NvDirectMode
{
    // Install vtable patches for swap-chain creation slots. Idempotent —
    // subsequent calls with already-patched factories are no-ops. r2 may
    // be null if IDXGIFactory2 isn't exposed (older runtime / older
    // game's CreateDXGIFactory call); the IDXGIFactory base slots still
    // get patched in that case.
    bool InstallFactoryVtablePatch(IDXGIFactory* r0, IDXGIFactory2* r2);
}
