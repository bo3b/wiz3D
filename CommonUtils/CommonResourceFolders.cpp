#include "StdAfx.h"
#include "CommonResourceFolders.h"
#include <Shlobj.h>
#include <Shlwapi.h>
#include <string.h>
#include <tchar.h>
#include "..\Shared\ProductNames.h"

namespace iz3d
{

namespace resources
{

// Helper: get the directory containing this DLL (CommonUtils.lib code 
// compiled into S3DWrapperD3D9.dll)
static bool GetPortableDirectory(TCHAR path[MAX_PATH])
{
	HMODULE hSelf = NULL;
	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCTSTR)&GetPortableDirectory, &hSelf) && hSelf)
	{
		if (GetModuleFileName(hSelf, path, MAX_PATH))
		{
			TCHAR* slash = _tcsrchr(path, _T('\\'));
			if (slash)
			{
				*(slash + 1) = _T('\0');
				return true;
			}
		}
	}
	return false;
}

bool GetAllUsersDirectory(TCHAR path[MAX_PATH])
{
	// Portable first: check if BaseProfile.xml exists next to our DLL
	if (GetPortableDirectory(path))
	{
		TCHAR testPath[MAX_PATH];
		_tcscpy(testPath, path);
		PathAppend(testPath, _T("BaseProfile.xml"));
		if (GetFileAttributes(testPath) != INVALID_FILE_ATTRIBUTES)
			return true;
	}
	// Fall back to system-wide ProgramData path
	bool ret = true;
	if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL, 0, path))) 
		PathAppend(path, _T(PRODUCT_NAME) _T("\\") );
	else
		ret = false;
	return ret;
}

bool GetLanguageFilesPath(TCHAR path[MAX_PATH])
{
	if(GetAllUsersDirectory(path)) 
	{
		PathAppend(path, _T("Language") _T("\\") );
		return true;
	}
	return false;
}

bool GetCurrentUserDirectory(TCHAR path[MAX_PATH])
{
	bool ret = true;
	if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path))) 
		PathAppend(path, _T(PRODUCT_NAME) _T("\\") );
	else
		ret = false;
	return ret;
}

}
}