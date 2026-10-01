# Native renderer: plan and state

Goal: render the whole game natively on Vulkan (Linux / Steam Deck, Windows, later
Android) with none of the Xenia-derived GPU emulation (EDRAM emulation, render target
cache, texture cache, shared-memory mirror machinery) in the shipped build.

## Design

The target architecture. It is reached by converting the existing Vulkan backend piece by
piece (see Approach below) rather than as a separate `graphics/native/` processor, but the
components are the same.

```text
recompiled game + statically linked XDK D3D (untouched)
  -> XDK command segments (PM4) in guest memory, kicked through the primary ring
  -> CommandProcessor base: packet decoding, register file, guest sync contract
     (fences, interrupts, WAIT_REG_MEM, swap, gamma ramp)
  -> native command processing: IssueDraw / IssueCopy / IssueSwap
       surfaces   host render targets keyed by EDRAM base + format + size + samples,
                  overlap model for aliasing (clear or reinterpret on rebind)
       resolves   copies into host textures keyed by guest address + format + size,
                  sampled directly; written back to guest memory only when the CPU reads
       textures   decoded once (untile, endian, format), revalidated by write watches
       shaders    Xenos microcode translated to SPIR-V (SDK translator, AOT + fallback)
       pipelines  exact state mapping, persistent pipeline cache
  -> VulkanPresenter (unchanged)
```

The command stream is the complete, exact source of GPU state for this game: the XDK
writes every draw's state into its command segment, and constants that bypass the D3D
device shadow (shader literals via LOAD_ALU_CONSTANT, GpuBeginShaderConstantF4, inline
SET_CONSTANT) only exist there. Reading it replaces the device-shadow capture of the
XDK-hook design used by other ReXGlue native ports, with the same result.

Invariants: never reorder, merge, cull, drop or substitute a submitted draw; map state
exactly; keep game-visible semantics (pixel centers, PWL gamma, EDRAM aliasing, NaN-as-zero
constants, MSAA sample counts, resolve tiling when the guest reads resolved memory).

## XDK D3D map (xdk_sigs match against the Conan-era signature set: 174 exact)

| Function | Address |
|---|---|
| D3DDevice_DrawVertices | 0x8244CF78 |
| D3DDevice_DrawIndexedVertices | 0x8244D360 |
| D3DDevice_DrawVerticesUP | 0x8244C910 |
| D3DDevice_BeginVertices / EndVertices | 0x8244C450 / 0x8244CEA0 |
| D3DDevice_Clear / D3D_InternalClearDraw | 0x82453C30 / 0x82453B08 |
| D3DDevice_Resolve | 0x82455570 |
| D3DDevice_Swap | 0x824544F0 |
| D3DDevice_BeginTiling / EndTiling / SetPredication | 0x824675C8 / 0x82467B50 / 0x82467450 |
| D3DDevice_CreateShaderA / B (container) | 0x82448178 / 0x82448308 |
| D3DDevice_SetVertexShader / SetPixelShader | 0x82445278 / 0x82445578 |
| D3DDevice_SetTexture / CreateTexture | 0x824408E0 / 0x82440578 |
| D3DVertexBuffer_Lock / Unlock | 0x824418A0 / 0x824419A0 |
| D3D_KickOff / RingMakeSpace / RingAlloc | 0x824572F8 / 0x82457CC8 / 0x82457F50 |
| D3D_AddCallsToPrimaryBuffer (INDIRECT_BUFFER) | 0x82456E68 |
| D3D_BlockOnFence / PollGpuProgress / BlockUntilIdle | 0x824574B8 / 0x82452018 / 0x82458080 |
| D3D_LoadShaderLiterals (LOAD_ALU_CONSTANT) | 0x8245F810 |
| D3DDevice_GpuBeginShaderConstantF4 | 0x82444BD8 |
| D3D_BeginVizQuery / EndVizQuery | 0x8244EEA8 / 0x8244EF78 |

Device layout matches the Conan-era XDK notes (command segment write pointer at
device+0x30).

## Approach (since 2026-10-01)

The Vulkan backend is converted in place: one emulation piece at a time is replaced by a
native one, each step verified bit-identical on the captured frames and shippable on its own,
instead of building a separate renderer next to it. Stages, in order:

1. **Shaders ahead of time** - done. Every shader the game uses is translated before play;
   the runtime serves `native_shaders/` (hand-written natives) and `native_shaders/translated/`
   (exported translations, used only when the translator source hash and the GPU configuration
   match the ones they were made with). Nothing is translated during play in the captured scenes.
2. **Native resolves everywhere** - done for every resolve in the captured scenes. Resolves
   into textures are drawn directly (lazy memory write-back at 2x); resolves no texture reads
   yet write the memory directly from the render target. The EDRAM buffer dump and compute
   resolve remain only as the fallback for unsupported cases (MSAA sources, exponent bias,
   gamma, non-bitwise-equivalent formats).
3. **Render targets as surfaces** - next. Host render targets sized by use instead of EDRAM
   row coverage, a resolution scale per target, no EDRAM address limits on size. Unlocks any
   aspect ratio and resolution (with game-side camera and HUD changes), per-target resolution
   (shadow maps), less memory, fewer transfers.
4. **Geometry without emulation tricks** - rectangle lists and point sprites without geometry
   shaders (Mali GPUs on Android have none), real vertex and index buffers instead of shaders
   reading the guest memory mirror.
5. **Memory** - no full guest memory mirror on the GPU and no write watching; textures and
   buffers uploaded when the game loads or changes them (hooks on its resource code).
6. Removing the emulation paths, Windows check, Android port.

## Validation

- Offline A/B: `tools/bench/replay_ab.py` replays captured frames through renderer variants
  headlessly, diffs the images and reports GPU time per category. Build the replayer with
  `tools/native-renderer/build_trace_reference.sh`.
- Captures: `tools/bench/autorun.py` drives the game unattended (injected pad input, engine
  screenshots, perf, GPU clock/power) and `trace <label>` grabs a frame trace.
- New GPU-visible code runs on llvmpipe first (`VK_ICD_FILENAMES=.../lvp_icd.x86_64.json`):
  a GPU fault on the Steam Deck resets the GPU and can black out the session. llvmpipe can
  replay 1x traces but not 2x ones yet.
- Debug modes that prove exactness: `native_resolve_debug_reload` (textures reload from the
  memory native resolves write), `native_resolve_debug_memory_only_all` (every resolve writes
  only the memory), `native_resolve_debug_verify_stencil_capture`.

## State (2026-10-01)

- Shaders: `aot_export_path` exports runtime translations in the served format;
  `TRACE_SHADER_STORAGE=<cache>:45410809` makes the replayer load a shader storage the way the
  game does at boot; `tools/native-renderer/build_translated_set.sh` builds
  `translated_shaders.tar.xz` (about 1 MB, both scales, with the plain / level 0 texture
  variants), which the release workflow unpacks into `native_shaders/`.
- EDRAM transfers: transfers into render targets cleared right after binding are held back
  over consecutive clears and done only outside the cleared area (the shadow map pass: 0.7 ms
  at 2x down to almost nothing).
- Stencil: depth resolves of an area whose stencil is known to be uniform (cleared, not
  written since) can skip capturing it (`native_resolve_uniform_stencil`, about 1 ms at 2x);
  off until its write-back path is validated.
- GPU time per frame on the Steam Deck, t09 gameplay trace: about 6.3 ms at 1x, 14.2 ms at 2x.
