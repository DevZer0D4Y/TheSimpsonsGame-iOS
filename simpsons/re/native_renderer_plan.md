# Native renderer: plan and state

Goal: render the whole game natively on Vulkan (Linux / Steam Deck, Windows, later
Android) with none of the Xenia-derived GPU emulation (EDRAM emulation, render target
cache, texture cache, shared-memory mirror machinery) in the shipped build.

## Design

```text
recompiled game + statically linked XDK D3D (untouched)
  -> XDK command segments (PM4) in guest memory, kicked through the primary ring
  -> CommandProcessor base: packet decoding, register file, guest sync contract
     (fences, interrupts, WAIT_REG_MEM, swap, gamma ramp)
  -> NativeCommandProcessor (graphics/native/): IssueDraw / IssueCopy / IssueSwap
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

## Milestones

1. Skeleton: NativeGraphicsSystem boots with no Xenia GPU components; guest sync
   contract holds (menus respond, audio, no hangs) with a black screen.
2. Menus and 2D: textures, blending, rect/quad lists.
3. Gameplay: 3D, depth/stencil, surface aliasing, resolves, post chain.
4. Parity: frame-by-frame A/B against the legacy renderer on scripted scenes; memexport
   skinning (characters).
5. Performance pass; native becomes the only renderer.
6. Windows check; Android port.

## Validation

`tools/bench/autorun.py` drives the game unattended (injected pad input, engine
screenshots, perf, GPU clock/power, audio dropouts through a null sink). Legacy vs native
screenshots at the same guest frame are the parity gate.

## State (2026-09-28)

- Native resolves: a resolve from a 1x host render target is drawn straight into the textures
  that sample the destination, writing the exact texel bits (integer view) the texture would
  load from the resolved memory. Bit-exact against the EDRAM path on 10 captured frames (menus,
  gameplay, pause); 8-12% less GPU time per frame. The EDRAM resolve still runs to keep guest
  memory coherent; skipping it (with writeback on demand) is the next step.
- Offline A/B: `tools/bench/replay_ab.py` replays captured frames through renderer variants
  headlessly, diffs the images and reports GPU time per category. Build the replayer with
  `tools/native-renderer/build_trace_reference.sh`.
- Captures: `autorun.py` `trace <label>` grabs a frame trace from a running game unattended.
- Character pop-in: character draws that read absent (all-zero) morph streams are vetoed on
  Vulkan (and would only run without rasterization when admitted). Rasterizing them renders the
  characters correctly on llvmpipe, but hung the Deck's GPU when replayed on it; the cause is not
  found yet. Never test those draws on the real GPU without a plan for a GPU reset.
- The precompiled shader set (`aot_shader_path`) is keyed by shader hash and variant only;
  regenerate it after any translator change or the game keeps serving the old modules.
