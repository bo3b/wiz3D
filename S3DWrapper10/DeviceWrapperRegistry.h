#pragma once

// Device-handle → wrapper lookup table.
//
// Why this exists: The original iZ3D approach allocated D3DDeviceWrapper
// inside the runtime's private device buffer (offset 0) and shifted the
// device handle the driver sees by sizeof(wrapper). This worked on Win7-10
// where the D3D11 DDI struct layout was stable. On Win11 the runtime hands
// drivers a D3D11.10 (or later) struct that has more function pointers than
// our compiled D3D11.0 layout knows about. Slots we don't wrap get called by
// the runtime with the unshifted handle, but the driver wrote its data at
// hDevice + wrapperSize → null function pointer crashes inside nvldumd.dll.
//
// Fix: stop shifting. Allocate the wrapper on the heap. Look it up via this
// registry, keyed by the runtime's hDevice.pDrvPrivate.

#include <windows.h>

namespace iZ3D
{
	// Fixed-size array — virtually all DX10/11 games create 1 device.
	// Linear scan over 16 entries is faster than a hashmap, no STL alloc
	// during DllMain, and simpler. Bumps to map only if this overflows.
	const int kDeviceRegistryCapacity = 16;

	struct DeviceRegistryEntry
	{
		void* pHandle;   // hDevice.pDrvPrivate (runtime's key)
		void* pWrapper;  // D3DDeviceWrapper* or D3DMonoDeviceWrapper*
	};

	extern DeviceRegistryEntry g_deviceRegistry[kDeviceRegistryCapacity];
	extern CRITICAL_SECTION    g_deviceRegistryCS;
	extern bool                g_deviceRegistryInitialized;

	void InitializeDeviceRegistry();
	void RegisterDeviceWrapper(void* pHandle, void* pWrapper);
	void UnregisterDeviceWrapper(void* pHandle);

	// Hot path — called from D3D10_GET_WRAPPER() on every wrapped DDI call.
	// Inline + linear scan: 16-entry array fits in 1-2 cache lines.
	inline void* LookupDeviceWrapper(void* pHandle)
	{
		if (!g_deviceRegistryInitialized) return NULL;
		EnterCriticalSection(&g_deviceRegistryCS);
		void* pWrapper = NULL;
		for (int i = 0; i < kDeviceRegistryCapacity; ++i)
		{
			if (g_deviceRegistry[i].pHandle == pHandle)
			{
				pWrapper = g_deviceRegistry[i].pWrapper;
				break;
			}
		}
		LeaveCriticalSection(&g_deviceRegistryCS);
		return pWrapper;
	}
}
