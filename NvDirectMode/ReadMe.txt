wiz3D — 3D Vision Direct Mode proxies
======================================

These DLLs are for games that ALREADY render their own stereo content via
NVIDIA's 3D Vision Direct Mode API (NvAPI_Stereo_SetActiveEye + per-eye
render passes). The proxies route the game's two eye renders into a
side-by-side surface so a downstream stereo display can present them.

If you have a MONO game and want to add stereo support to it, you do NOT
want this — you want the regular wiz3D wrappers in the dx9/dx10-11/opengl/
sibling folders, which perform active stereoization.

------------------------------------------------------------------
What goes where
------------------------------------------------------------------
Each leaf folder contains TWO DLLs that should be installed together into
the game's executable folder (where the .exe lives):

  dx9/x86/      d3d9.dll      + nvapi.dll        (32-bit DX9 game)
  dx9/x64/      d3d9.dll      + nvapi64.dll      (64-bit DX9 game)
  dx10/x86/     d3d10.dll     + nvapi.dll        (32-bit DX10 game)
  dx10/x64/     d3d10.dll     + nvapi64.dll      (64-bit DX10 game)
  dx11/x86/     d3d11.dll     + nvapi.dll        (32-bit DX11 game)
  dx11/x64/     d3d11.dll     + nvapi64.dll      (64-bit DX11 game)
  opengl/x86/   opengl32.dll  + nvapi.dll        (32-bit OpenGL game)
  opengl/x64/   opengl32.dll  + nvapi64.dll      (64-bit OpenGL game)

The proxy DLL handles per-eye render-target routing; nvapi[64].dll handles
the NvAPI_Stereo_* calls and shares active-eye state with the proxy. Both
files must be in the game folder for routing to work.

To uninstall, simply delete the two DLLs from the game folder.

------------------------------------------------------------------
Test targets
------------------------------------------------------------------
  Max Payne 3      launch with -stereo 1 (DX9 path)
  Tutorial07       3Dmigoto sample (DX11 path)
  (any DX10 game)  no widely-known native Direct Mode DX10 game
  (any GL game)    no widely-known native Direct Mode OpenGL game

------------------------------------------------------------------
Known limitations (current build)
------------------------------------------------------------------
  * Windowed mode only on DX paths. Fullscreen requests an invalid
    doubled-width display mode and CreateDevice will fail.
  * Reset / ResizeBuffers do not re-double — runtime window resize
    breaks the doubled-buffer invariant.
  * DXGI factory path (CreateDXGIFactory -> CreateSwapChain) is not
    intercepted on DX11 — only D3D11CreateDeviceAndSwapChain is. Games
    that use the factory path won't get their swap chain doubled.
  * ID3D10Device1 / ID3D11Device1+ pass through unwrapped.
  * OpenGL: window resize, MSAA, and multi-context (wglShareLists) are
    not handled.
  * No SR weaver / native 3D Vision display path is wired up yet — the
    doubled side-by-side surface goes to the OS swap chain, which on a
    non-stereo monitor will squash both eyes into the window. The
    routing is correct; the final stereo display step is future work.

------------------------------------------------------------------
Diagnostic logs
------------------------------------------------------------------
At runtime the proxy writes to nvdirectmode_proxy.log next to the game
exe (shared filename across DX9/DX10/DX11/OpenGL — only one will load
per game). On a fatal exception, nvdirectmode_crash.dmp + a stack trace
in the log are produced.
