#include "stdafx.h"
#include "DestroyDevice.h"
#include "..\Streamer\CodeGenerator.h"
#include "D3DDeviceWrapper.h"
#include "..\DeviceWrapperRegistry.h"

namespace Commands
{

	void DestroyDevice::Execute( D3DDeviceWrapper *pWrapper )
	{
		BEFORE_EXECUTE(this);
		//pWrapper->OriginalDeviceFuncs.pfnDestroyDevice(pWrapper->hDevice);
		AFTER_EXECUTE(this); 
	}

	bool DestroyDevice::WriteToFile( D3DDeviceWrapper *pWrapper ) const
	{
		WriteStreamer::CmdBegin( pWrapper->GetWrapperHandle(), ( Command* )this,  "DestroyDevice" );
		WriteStreamer::CmdEnd();
		DESTROY_RESOURCE(pWrapper->GetWrapperHandle());
		return true;
	}

	bool DestroyDevice::ReadFromFile()
	{
		D3D10DDI_HDEVICE hDevice = { NULL };
		ReadStreamer::CmdBegin( hDevice );
		ReadStreamer::CmdEnd();
		return true;
	}
}

VOID (APIENTRY  DestroyDevice)(D3D10DDI_HDEVICE  hDevice)
{
	// Resolve the wrapper once. D3D10_GET_WRAPPER does a side-table lookup
	// under A2; the wrapper destructor itself contains code that calls
	// D3D10_GET_WRAPPER (e.g. ExternalConstantBuffer::~ResourceWrapper at
	// D3DDeviceWrapper.cpp:203), so the wrapper must remain REGISTERED in
	// the side-table while the destructor runs. Unregister + free only
	// after the destructor and the driver's pfnDestroyDevice have both
	// finished.
	D3DDeviceWrapper* pWrapper = D3D10_GET_WRAPPER();

#ifndef EXECUTE_IMMEDIATELY_G2
	Commands::DestroyDevice* command = new Commands::DestroyDevice();
	pWrapper->AddCommand( command, true );
#else
	pWrapper->ProcessCB();
#endif

	// Cache the driver-side destroy ptr and handle before destructing.
	auto pfnDestroy = pWrapper->OriginalDeviceFuncs.pfnDestroyDevice;
	D3D10DDI_HDEVICE hDrvDevice = pWrapper->hDevice;

	// Match the original ordering: explicit destructor first (cleans up
	// stereo resources while the device is still alive), then pfnDestroyDevice.
	pWrapper->~D3DDeviceWrapper();

	{
		//THREAD_GUARD_D3D10();								// Don't call that, CS in not valid anymore
		pfnDestroy( hDrvDevice );
	}

	iZ3D::UnregisterDeviceWrapper(hDevice.pDrvPrivate);
	::operator delete(pWrapper);
}

