#include "stdafx.h"
#include "ShaderOverrideLoader.h"
#include "..\CommonUtils\System.h"
#include "Trace.h"

#include <Shlwapi.h>
#include <vector>
#include <tchar.h>

#pragma comment(lib, "Shlwapi.lib")

namespace wiz3d
{
namespace shader_override
{

DWORD ComputeShaderCRC32(const void* bytecode, UINT sizeBytes)
{
	return CalculateCRC32(bytecode, sizeBytes);
}

// Cache the ShaderOverride/ folder presence check. The folder either exists
// for the lifetime of the process (user installed a fix before launching)
// or it doesn't (cleanest baseline). We never need to re-probe.
static bool g_bShaderFixActive   = false;
static bool g_bShaderFixChecked  = false;

// Returns the directory containing the wrapper DLL itself (which is the
// game folder when wiz3D is installed per-game, the standard portable layout).
// Output ends with a trailing backslash.
static bool GetWrapperDirectory(TCHAR path[MAX_PATH])
{
	HMODULE hSelf = NULL;
	if (!GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCTSTR)&GetWrapperDirectory, &hSelf))
		return false;
	if (!hSelf)
		return false;
	if (!GetModuleFileName(hSelf, path, MAX_PATH))
		return false;
	TCHAR* slash = _tcsrchr(path, _T('\\'));
	if (!slash)
		return false;
	*(slash + 1) = _T('\0');
	return true;
}

static bool BuildOverridePath(DWORD crc32, ShaderType type, TCHAR pathOut[MAX_PATH])
{
	if (!GetWrapperDirectory(pathOut))
		return false;
	PathAppend(pathOut, _T("ShaderOverride"));
	PathAppend(pathOut, type == ShaderType::Vertex ? _T("VertexShaders") : _T("PixelShaders"));
	TCHAR fname[32];
	_stprintf_s(fname, 32, _T("%08X.txt"), crc32);
	PathAppend(pathOut, fname);
	return true;
}

bool IsShaderFixActive()
{
	if (g_bShaderFixChecked)
		return g_bShaderFixActive;

	TCHAR dir[MAX_PATH] = { 0 };
	if (GetWrapperDirectory(dir))
	{
		PathAppend(dir, _T("ShaderOverride"));
		DWORD attr = GetFileAttributes(dir);
		g_bShaderFixActive = (attr != INVALID_FILE_ATTRIBUTES) &&
		                     ((attr & FILE_ATTRIBUTE_DIRECTORY) != 0);
	}
	g_bShaderFixChecked = true;
	if (g_bShaderFixActive)
		DEBUG_MESSAGE(_T("[ShaderOverride] folder present - shader-fix active\n"));
	return g_bShaderFixActive;
}

bool TryLoadOverride(DWORD crc32, ShaderType type, CComPtr<ID3DXBuffer>& pAssembledOut)
{
	if (!IsShaderFixActive())
		return false;
	TCHAR path[MAX_PATH] = { 0 };
	if (!BuildOverridePath(crc32, type, path))
		return false;

	HANDLE hFile = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL,
	                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false; // no override for this CRC; expected for most shaders

	DWORD fileSize = GetFileSize(hFile, NULL);
	if (fileSize == INVALID_FILE_SIZE || fileSize == 0)
	{
		CloseHandle(hFile);
		return false;
	}

	std::vector<char> buf(fileSize + 1);
	DWORD bytesRead = 0;
	BOOL ok = ReadFile(hFile, buf.data(), fileSize, &bytesRead, NULL);
	CloseHandle(hFile);
	if (!ok || bytesRead != fileSize)
		return false;
	buf[bytesRead] = '\0';

	CComPtr<ID3DXBuffer> pErrors;
	HRESULT hr = D3DXAssembleShader(buf.data(), bytesRead,
	                                 NULL, /* defines */
	                                 NULL, /* include handler */
	                                 0,    /* flags */
	                                 &pAssembledOut,
	                                 &pErrors);
	if (FAILED(hr))
	{
		// Log the error message from the assembler so the user can see what
		// went wrong with their fix file.
		const char* errText = pErrors ? (const char*)pErrors->GetBufferPointer() : "<no diagnostic>";
		DEBUG_MESSAGE(_T("[ShaderOverride] D3DXAssembleShader failed for CRC %08X: 0x%08x\n%S\n"),
		              crc32, hr, errText);
		return false;
	}

	DEBUG_MESSAGE(_T("[ShaderOverride] Loaded %s replacement for CRC %08X (%u bytes)\n"),
	              type == ShaderType::Vertex ? _T("VS") : _T("PS"),
	              crc32, pAssembledOut->GetBufferSize());
	return true;
}

} // namespace shader_override
} // namespace wiz3d
