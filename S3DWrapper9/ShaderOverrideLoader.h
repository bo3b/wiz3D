// wiz3D - HelixMod-compatible ShaderOverride loader.
//
// Looks for replacement shader assembly at:
//   <game_folder>\ShaderOverride\<VertexShaders|PixelShaders>\<CRC32_HEX>.txt
//
// If found, runs D3DXAssembleShader on the file contents and returns the
// resulting bytecode for the wrapper to substitute in place of the original.
//
// File format is HelixMod's: a Microsoft D3DX9 shader-disassembly text file,
// optionally with a leading comment block describing the source HLSL. We pass
// the whole file to D3DXAssembleShader, which accepts // comments natively.
//
// We don't ship any HelixMod files. Users place them in the game folder.

#pragma once

#include <atlbase.h>
#include <d3dx9.h>

namespace wiz3d
{
namespace shader_override
{

enum class ShaderType { Vertex, Pixel };

// Computes the CRC32 wiz3D uses for shader fingerprinting (matches the algorithm
// in CommonUtils/System.cpp::CalculateCRC32). Caller passes the original
// (unassembled) bytecode buffer.
DWORD ComputeShaderCRC32(const void* bytecode, UINT sizeBytes);

// Try to load a replacement shader for the given CRC. Returns true and fills
// pAssembledOut on success; returns false (no error logged) if the override
// file simply doesn't exist. Logs a warning and returns false on assembly
// failure (caller falls through to the original shader).
bool TryLoadOverride(DWORD crc32, ShaderType type, CComPtr<ID3DXBuffer>& pAssembledOut);

// True if a ShaderOverride/ folder exists in the wrapper's directory.
// Cached after first call. The wider shader-fix machinery (texldl scan,
// stereo-params texture binding, override file lookup) gates on this so
// games without HelixMod-style fixes installed get exactly the v0.1.5
// behaviour with no side effects from the new code paths.
bool IsShaderFixActive();

} // namespace shader_override
} // namespace wiz3d
