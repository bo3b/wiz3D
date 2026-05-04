# wiz3D Proxy DLLs

Proxy DLL loaders for the iZ3D stereo driver. Each proxy replaces a system
graphics DLL in a game's folder, forwards all calls to the real system DLL,
and loads the corresponding iZ3D wrapper to enable stereoscopic 3D.

## Proxy Targets

| Subfolder   | Output DLL      | Hooks                      | Loads Wrapper           |
|-------------|-----------------|----------------------------|-------------------------|
| `d3d9/`     | `d3d9.dll`      | `Direct3DCreate9(Ex)`      | `S3DWrapperD3D9.dll`    |
| `d3d8/`     | `d3d8.dll`      | `Direct3DCreate8`          | `S3DWrapperD3D8.dll`    |
| `ddraw/`    | `ddraw.dll`     | `DirectDrawCreateEx`       | `S3DWrapperD3D7.dll`    |
| `dxgi/`     | `dxgi.dll`      | `CreateDXGIFactory(1/2)`   | `S3DWrapperD3D10.dll`   |
| `opengl32/` | `opengl32.dll`  | `wglCreateContext` etc.    | `S3DWrapperOGL.dll`     |

## How It Works

1. Game loads `d3d9.dll` (or other proxy) from its own directory
2. Proxy loads the **real** system DLL from `C:\Windows\System32\`
3. All exports are forwarded transparently to the real DLL
4. Key creation functions are intercepted to load the iZ3D wrapper DLL
5. The wrapper takes over rendering and applies stereo 3D

## Usage

Copy the proxy DLL + wrapper DLL + output plugins into the game folder.
