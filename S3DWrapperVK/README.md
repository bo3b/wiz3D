# S3DWrapperVK

Vulkan stereo wrapper. Shares all logic with the DX12 wrapper via a single ReShade Add-on backend — only the proxy DLL entry point (`vulkan-1.dll`) and the Vulkan implicit-layer manifest (`wiz3D.json`) differ from `S3DWrapper12/`.

See **[`../S3DWrapper12/PLAN.md`](../S3DWrapper12/PLAN.md)** for the full architecture, roadmap, and technical plan covering both DX12 and Vulkan together.
