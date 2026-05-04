#include "stdafx.h"
#include "DeviceWrapperRegistry.h"

namespace iZ3D
{
	DeviceRegistryEntry g_deviceRegistry[kDeviceRegistryCapacity] = {};
	CRITICAL_SECTION    g_deviceRegistryCS;
	bool                g_deviceRegistryInitialized = false;

	void InitializeDeviceRegistry()
	{
		if (g_deviceRegistryInitialized) return;
		InitializeCriticalSection(&g_deviceRegistryCS);
		for (int i = 0; i < kDeviceRegistryCapacity; ++i)
		{
			g_deviceRegistry[i].pHandle = NULL;
			g_deviceRegistry[i].pWrapper = NULL;
		}
		g_deviceRegistryInitialized = true;
	}

	void RegisterDeviceWrapper(void* pHandle, void* pWrapper)
	{
		if (!g_deviceRegistryInitialized) InitializeDeviceRegistry();
		EnterCriticalSection(&g_deviceRegistryCS);
		int free = -1;
		for (int i = 0; i < kDeviceRegistryCapacity; ++i)
		{
			if (g_deviceRegistry[i].pHandle == pHandle)
			{
				g_deviceRegistry[i].pWrapper = pWrapper;
				LeaveCriticalSection(&g_deviceRegistryCS);
				return;
			}
			if (free < 0 && g_deviceRegistry[i].pHandle == NULL) free = i;
		}
		if (free >= 0)
		{
			g_deviceRegistry[free].pHandle = pHandle;
			g_deviceRegistry[free].pWrapper = pWrapper;
		}
		LeaveCriticalSection(&g_deviceRegistryCS);
	}

	void UnregisterDeviceWrapper(void* pHandle)
	{
		if (!g_deviceRegistryInitialized) return;
		EnterCriticalSection(&g_deviceRegistryCS);
		for (int i = 0; i < kDeviceRegistryCapacity; ++i)
		{
			if (g_deviceRegistry[i].pHandle == pHandle)
			{
				g_deviceRegistry[i].pHandle = NULL;
				g_deviceRegistry[i].pWrapper = NULL;
				break;
			}
		}
		LeaveCriticalSection(&g_deviceRegistryCS);
	}
}
