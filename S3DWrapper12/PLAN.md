# wiz3D DX12 / Vulkan Stereo Wrapper Plan

Status: design document, no code yet. This plan covers DX12 and Vulkan together because the same ReShade-based backend handles both APIs through one codebase.

## Goals

- Re-enable stereoscopic 3D in modern DX12 and Vulkan games.
- Drop-in proxy DLL pattern, matching the rest of wiz3D. User copies files into the game folder, no installer, no host program.
- Avoid Unreal Engine and RE games as UEVR and REFramework cover those. 
- Cover the gap those tools leave: Unity, Godot, custom-engine indie titles, and AAA games with neither HelixMod nor UEVR support.
- Maintain LGPL v2.1 license compatibility throughout.

## Non-goals

- Universal automatic stereo on every DX12/Vulkan game. That's the marketing fantasy. Realistic outcome: 30–50% of modern games work cleanly via heuristics + engine hooks; another 20–30% work with per-game tweaking; the rest need manual shader fixes or don't work.
- Bypassing anti-cheat. wiz3D will be a proxy DLL that hooks command lists; Easy Anti-Cheat, BattlEye, Vanguard, Denuvo will flag it. Single-player and offline only, accepted as a permanent limitation.
- Bypassing DRM. Same situation, same answer.
- Forking ReShade itself. The Add-on SDK route gives 95% of the access for ~5% of the maintenance burden.
- Kernel-mode drivers, system service installation, or anything that requires admin rights at runtime.
- DX12 Frame Generation (DLSS 3/4 Frame Gen, FSR 3 Frame Gen) compatibility. Detect and disable. Not worth the engineering to fix.

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  Game (DX12 or Vulkan)                                      │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│  Proxy DLL (dxgi.dll for DX12, vulkan-1.dll for Vulkan)     │
│  • Loads real system DLL                                    │
│  • Loads ReShade core (BSD 3-Clause, statically linked)     │
│  • Initialises wiz3D add-on                                 │
└─────────────────┬───────────────────────────────────────────┘
                  │
                  ▼
┌─────────────────────────────────────────────────────────────┐
│  ReShade Core (unmodified, statically linked)               │
│  • Provides reshade::api unified abstraction over           │
│    DX12 and Vulkan command queues, descriptors, draws       │
│  • UI / shader compiler / .fx loader stripped out by build  │
│    flags so the binary is lean                              │
└─────────────────┬───────────────────────────────────────────┘
                  │  reshade::api events
                  ▼
┌─────────────────────────────────────────────────────────────┐
│  wiz3D Add-on (the new code)                                │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ Engine Detection Layer                              │    │
│  │  • Unreal Engine 4/5: read FMinimalViewInfo via     │    │
│  │    pattern scan + RTTI (UEVR-style approach)        │    │
│  │  • Unity: read Camera.main via Mono runtime hooks   │    │
│  │  • Godot: read Camera3D via Godot binding hooks     │    │
│  │  • Unknown engine: fall through to heuristic        │    │
│  └─────────────────────────────────────────────────────┘    │
│                          │                                  │
│                          ▼                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ Matrix Manager (heuristic, generic fallback)        │    │
│  │  • Scans constant buffers / push constants /        │    │
│  │    uniform buffers for projection-matrix footprint  │    │
│  │  • Scores candidates (aspect ratio, target          │    │
│  │    resolution, shader stage, compute vs vertex)     │    │
│  │  • Locks onto winner only after stable consensus    │    │
│  │  • Re-scans on cutscenes / camera switches          │    │
│  └─────────────────────────────────────────────────────┘    │
│                          │                                  │
│                          ▼                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ Stereo Render Coordinator                           │    │
│  │  • Two paths:                                       │    │
│  │    1. Camera-object override (engine-known): patch  │    │
│  │       camera struct, let game render twice          │    │
│  │    2. Vertex shader injection: append clip-space    │    │
│  │       offset (Geo-11 hybrid) using clean matrix     │    │
│  │       found by Matrix Manager for FOV/depth         │    │
│  │  • Calls back into ReShade to issue per-eye draws   │    │
│  └─────────────────────────────────────────────────────┘    │
│                          │                                  │
│                          ▼                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ State Machine — 2D vs 3D                            │    │
│  │  • Boot: heuristic active, no clean matrix yet      │    │
│  │  → Dumb Duplicator (50% horizontal squish + repeat) │    │
│  │  • 3D Trigger: matrix locked, switch to true stereo │    │
│  │  • UI / orthographic / depth-disabled draws → keep  │    │
│  │    Dumb Duplicator path so HUD stays at screen      │    │
│  │    depth                                            │    │
│  └─────────────────────────────────────────────────────┘    │
│                          │                                  │
│                          ▼                                  │
│  ┌─────────────────────────────────────────────────────┐    │
│  │ Output Method                                       │    │
│  │  • SBS / Half-SBS / Top-Bottom / Frame-Sequential   │    │
│  │  • Anaglyph variants                                │    │
│  │  • SR Weave (when DX10/11 SR weave Output method    │    │
│  │    is ported, parallel work)                        │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
```

The "iZ3D math" we keep is small: off-axis frustum asymmetry, separation/convergence formulas, the X-axis matrix shift. Roughly a few hundred lines lifted from the DX9 wrapper and made API-agnostic. Everything else is new code.

## License posture

- **ReShade** (BSD 3-Clause) — compatible with LGPL v2.1. Statically linked, copyright notice preserved.
- **Geo3D** (Flugan, BSD 2-Clause) — compatible. Worth studying for ReShade-API patterns, but it's per-game-profile based, not heuristic — limited reuse.
- **REFramework** (MIT) — compatible. Useful reference for safe DX12 command-list interception in multi-threaded engines.

## Compatibility tiers

The honest expectation curve:

| Tier | Coverage | What works |
|---|---|---|
| Engine-known (UE/Unity/Godot hook works) | 50–70% of titles in those engines | True dual-view stereo with low setup |
| Heuristic-only (custom engine, clean matrix found) | Maybe 30% of remaining games | Stereo works, may have UI/post-process glitches |
| Manual shader fix needed | Long tail | Per-game work; community contribution territory |
| Won't work | Anti-cheat protected, DRM-locked, broken-by-design | Accepted, documented, moved past |

iZ3D's DX9 era worked on hundreds of games but absolutely not all of them. iZ3D's DX10/11 attempt covered a much smaller fraction. wiz3D for DX12/Vulkan reaching anywhere near DX10/11 coverage levels would be a huge contribution.

## Technical hurdles & mitigations

### 1. WVP pre-multiplication

Modern engines pre-multiply World × View × Projection on the CPU before sending to GPU. The clean projection matrix the heuristic depends on may never appear in the constant buffers used for geometry.

Mitigations, in priority order:
- **Engine-aware path** — for UE/Unity/Godot, read the camera object directly before the engine's render thread does the multiplication. No matrix hunting needed.
- **Compute shader stage scan** — SSAO, SSR, volumetric fog, screen-space lighting passes draw onto a fullscreen quad and need the *unmultiplied* projection (or its inverse) passed in a constant buffer, since there's no World matrix to fold in. Scan compute and pixel shader CBs preferentially.
- **Inverse projection fingerprint** — DXR/RT raygen, deferred lighting, and TAA passes commonly bind `Inverse(VP)` for screen-to-world reconstruction. Inverse matrices have their own footprint (m44 = 1, distinctive m11/m22 ratios). Find IVP, invert it back to recover VP.
- **Skybox draw call** — most engines render the skybox with translation stripped from the View matrix and no World matrix. The CB bound to that draw is essentially pure VP. Heuristic: identify "fullscreen-ish, no depth write, drawn early" passes.
- **Depth-buffer aspect ratio match** — match candidate matrices' (m22 / m11) ratio against (window_width / window_height). Strongest single signal for "this is the camera, not a shadow map."

### 2. Reverse-Z

Most modern AAA DX12 games use Reverse-Z depth (near=1, far=0) to dodge Z-fighting. The fingerprint check has to accept both regular-Z and Reverse-Z layouts.

Test: m33 ≈ 0 with m43 small positive → Reverse-Z. m33 small positive with m43 ≈ 0 → standard. Either passes the projection-matrix test; both yield valid `Inverse(P)` for stereo offset.

### 3. Motion vectors with TAA / DLSS / FSR / XeSS

Upscalers need per-pixel motion vectors. Game writes vectors based on one camera. If we render two eyes with different cameras, the motion vectors are wrong for the second eye → upscaler smears.

Mitigations:
- **Per-eye motion vector buffer** — intercept the motion vector pass. For the right eye, apply the same horizontal stereo shift to the vector data that we applied to the camera.
- **Per-eye upscaler invocations** — DLSS/FSR/XeSS expose API for input/output buffer pairs. Run the upscaler twice per frame, once per eye, with the corrected motion vectors.
- **Disable Frame Gen (DLSS 3/4, FSR 3 FG)** — Frame Gen interpolates between adjacent frames assuming temporal continuity. Our L→R "motion" looks to it like a violent sub-frame camera jump. Result is interpolated mid-eye morphs that flicker. Detect and force off.
- **TAA-only (no upscaler) games** — same motion vector approach but writing to the TAA history buffer instead of upscaler input. Some games allow disabling TAA in settings — easiest fix.

### 4. Ray tracing and path tracing

Hybrid raster + RT (most current AAA): rasterized geometry uses pre-multiplied WVP we may or may not be able to find. RT effects (reflections, GI, shadows) use the inverse VP we can find via compute-stage heuristic. Risk: RT lighting gets stereo-correctly offset but the rasterized geometry it lights doesn't move → mismatch where shadows fall in the wrong place per eye. *Probably worse than no RT for stereo*.

Pure path tracing (Cyberpunk PT mode, Quake 2 RTX, Portal RTX, Black Myth Wukong PT mode): everything goes through the RT pipeline. The raygen shader's inverse-VP IS the camera, period. Modify it for L/R eye and rays shoot from each eye position. Rasterized G-buffer intermediates may exist but are screen-space — they re-derive depth from the depth buffer, which is itself generated by the rays. Genuinely tractable — possibly more tractable than DX12 rasterization in the same game.

The catch is denoiser temporal accumulation. Denoisers (NRD, ReSTIR-flavored) accumulate radiance estimates across frames. Alternating L/R frames poisons the accumulation. Two options:
- Render both eyes within the same engine-frame (separate command lists, separate accumulation buffers per eye) — hard, requires deep engine cooperation
- Force the denoiser into "no-history" mode for first N frames after camera change — works at a quality cost, possibly the realistic option

Honest read: pure path tracing is 50/50 feasible with significant engineering. Hybrid RT is 20/80 — likely not worth the effort for a long time. Plain rasterization should be the focus until pure-PT support is the only thing left to do.

### 5. Multi-threaded command list recording

DX12 and Vulkan allow recording command lists on multiple worker threads simultaneously. wiz3D needs to be thread-safe in every hook callback. UEVR's "synchronized sequential" mode (force eye-1 to fully complete before eye-2 begins) is the conservative path — slower but stable.

Optimisation later: per-thread command list state, atomic descriptor updates, lock-free matrix manager queries. Don't optimise prematurely.

### 6. UI / 2D

Solved by the State Machine "Dumb Duplicator" approach. Identify orthographic projection / depth-disabled / late-pipeline draws. Draw them once into a fullscreen pre-buffer, then 50%-squish-and-paste to both eyes. UI sits at screen depth, looks fine, no per-game work.

Edge case: HUD elements that *should* float at world depth (target reticles in some games, world-space markers). These need per-game opt-out. Configurable in profile XML.

## Implementation roadmap

Each milestone lands as a working release with a known-supported game list. Don't move to the next until the current one has a stable demo.

### M0a: Passthrough plumbing — COMPLETE (2026-05-02)

- ✅ New `S3DWrapper12/` and `S3DWrapperVK/` projects
- ✅ `wiz3D-proxy/d3d12/d3d12.dll` proxy — forwards all 8 standard DX12 exports
- ✅ `wiz3D-proxy/vulkan-1/vulkan-1.dll` proxy — forwards 6 loader entry points
- ✅ `S3DWrapperD3D12.dll` and `S3DWrapperVK.dll` skeletons with the existing `InitializeExchangeServer` protocol, currently passthrough only. Loader-lock-safe DllMain, lazy init in `EnsureWrapperInitialized()` (deliberately mirroring the S3DWrapperD3D9 fix, so we don't repeat that deadlock on DX12/VK).
- ✅ Build chain validated: both proxies + both wrappers build clean Win32 + x64 standalone, no project references.

**Vulkan implicit-layer manifest (`wiz3D.json`) — INTENTIONALLY SKIPPED.** wiz3D is per-game-folder drop-in by design. The manifest route registers globally for every Vulkan app on the system, which works against that goal. A future contributor wanting to make a "universal Vulkan stereo injector" variant (auto-applies to every Vulkan game system-wide) could add the manifest with relatively little work — the DLL we ship from M0a already implements the layer-style passthrough chain. Out of scope for wiz3D itself.

### M0b: ReShade integration + Dumb Duplicator demo — pending

- ReShade vendored, statically linked, UI/effects stripped (build approach TBD — see ReShade decision below)
- ReShade Add-on skeleton: registers for `init_swapchain`, `present`, `bind_descriptor_tables`, `push_descriptors`, `push_constants`, `draw_indexed`, `draw`, `dispatch`
- Logging into existing wiz3D ZLOg infrastructure
- Output to existing wiz3D output methods (SBS/TAB/Anaglyph) by writing the final L/R into the swapchain's backbuffer pre-Present
- **Demo target**: any DX12 game's main menu rendered SBS via Dumb Duplicator. No real stereo yet. Proves the command-buffer interception works end-to-end.

M0b is the gate to M1 — the matrix scanner needs ReShade's command-list event hooks to have anything to scan.

### M1: Heuristic matrix scanner (2–3 months)

- `MatrixManager` class: candidate detection, scoring, consensus locking
- Compute-stage CB scan (highest confidence)
- Inverse-projection detection + back-inversion
- Reverse-Z support
- Skybox-draw-call heuristic
- Matrix Leaderboard UI overlay (debug-only) showing all candidates and scores
- **Demo target**: one specific custom-engine DX12 game (probably a small Unity DX12 title or something like Doom 2016/Eternal which is Vulkan) showing locked-on stereo via vertex shader injection.

### M2: Engine-aware paths (2–4 months, parallel)

- **Unity hook**: load Mono runtime, walk to `UnityEngine.Camera.main`, read view/projection. Doesn't depend on DLSS being present, works even on simple Unity indies.
- **Godot hook**: Godot is open-source — read `Camera3D` via known offsets; faster to implement than Unity.
- **Unreal Engine 4/5 hook**: pattern-scan for `UWorld`, walk to `ULocalPlayer::GetProjectionData`, read `FSceneView`. Alternatively, just document UEVR as the recommended path for UE games and don't reimplement.
- Engine detection: which game is running? Check executable signatures, loaded module names, process metadata.
- **Demo target**: 3–5 Unity games and 1–2 Godot games working with one-click stereo.

### M3: TAA / DLSS / FSR / XeSS support (2–3 months)

- Motion vector buffer detection (R16G16 / R16G16B16A16, screen-resolution, used as input to known upscaler API calls)
- Per-eye motion vector offset
- Per-eye upscaler invocation (DLSS3/FSR3 SDK calls intercepted, doubled)
- Frame Gen detection + automatic disable + user warning
- **Demo target**: one DLSS-using game working in stereo with DLSS Quality mode active.

### M4: Profile system + community fixes (ongoing)

- Per-game profile XML (matches the existing Config.xml/Profile system)
- Profile fields: separation, convergence defaults, UI opt-out lists, motion vector buffer hints, manual shader fix references
- Hook for HelixMod-style per-shader fixes when heuristics fail
- Community contribution path documented

### M5: Path tracing experiment (when M0–M4 stable)

- Pick one pure-PT game (Quake 2 RTX is open-source and Vulkan, ideal first target)
- Implement raygen-shader inverse-VP intercept
- Per-eye accumulation buffer separation
- Realistic outcome: one-game demo proving the technique, unclear how generalisable to closed-source AAA PT modes.

## Anti-cheat / DRM

Not addressed. wiz3D is a proxy DLL that hooks GPU command submission — anti-cheat will flag it. Documented as single-player only. Players accept the risk if they choose to use wiz3D in any game with anti-cheat. Same posture as ReShade itself.

## File organisation

```
S3DWrapper12/
  PLAN.md                  ← this file
  (future) wiz3D-d3d12.cpp
  (future) wiz3D-d3d12.vcxproj
  (future) MatrixManager.{h,cpp}
  (future) StereoCoordinator.{h,cpp}
  (future) Engines/
    Unreal.{h,cpp}
    Unity.{h,cpp}
    Godot.{h,cpp}
  (future) Heuristics/
    ProjectionFingerprint.{h,cpp}
    InverseProjection.{h,cpp}
    Skybox.{h,cpp}
  (future) ReShade/        ← submodule

S3DWrapperVK/
  README.md                ← one-liner: see ../S3DWrapper12/PLAN.md
  (future) wiz3D-vulkan.cpp
  (future) wiz3D-vulkan.vcxproj
```

The Vulkan project shares almost all code with the DX12 project — only the proxy DLL entry point differs. Single ReShade Add-on serves both. (`wiz3D.json` Vulkan implicit-layer manifest intentionally not shipped — see M0a note above.)

## Reality check, one more time

- **Will every modern AAA game work?** No. Anti-cheat alone rules out ~30% of the catalog. Custom engines with deeply pre-multiplied WVP and no compute-stage clean matrices add another chunk.
- **Will most Unity / Godot indie games work?** Probably yes, once the engine hooks are written.
- **Will Unreal 4/5 games work?** Probably yes for those without anti-cheat — UEVR proves the technique.
- **Will hybrid raster + RT games work in stereo with RT enabled?** Probably not cleanly. Recommend "RT off" until M5.
- **Will pure path-traced games work in stereo?** Maybe — biggest unknown, biggest potential win.
- **Will games with Frame Gen work?** Only with Frame Gen disabled.
- **Is this five years of work?** No. Two solid months gets M0 done. Six months gets M0+M1+M2 to working demos. The remaining milestones spread across whatever timescale works for the project.

The prize: any game working at all in DX12 stereo via a free, open-source, drop-in tool is a contribution the 3D community has been missing since NVIDIA killed 3D Vision in 2019.
