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
3. **Render targets as surfaces** - done, except copy-free resolves. Host render targets are
   the size of what the game draws to (`native_rt_size_by_use`: the rows of EDRAM tiles a
   target is drawn to, grown with a copy when more are needed) instead of covering the whole
   2048-tile EDRAM period: the main color and depth targets are 1280x720 (they were 1280x2048),
   the shadow map 1040x1024 - about 95 MB less video memory at 2x. Each target has its own
   resolution scale (`native_rt_original_resolution_targets`, the launcher's "Shadow
   resolution" option). Not done: letting a texture take over a target's image instead of
   copying it in a resolve (about 1 ms at 2x) - every color resolve in this game swaps red and
   blue, so the texture would need swizzled views, swap-aware memory write-back and proof that
   the next pass overwrites the whole target.
4. **Geometry without emulation tricks** - in progress. Rectangle lists, quad lists and point
   sprites without geometry shaders are exact (`vulkan_geometry_shader_primitives = false`, used
   automatically on GPUs without them, such as Mali); quads are split like the geometry
   shader's strip (`quad_list_triangle_order`). Next: real vertex and index buffers instead of
   shaders reading the guest memory mirror.
5. **Memory** - in progress. Uploads no longer wait for the GPU every draw: the per-frame
   reupload of every buffer (`clear_memory_page_state`) is off, and pages the game rewrites
   every frame are only uploaded when the bytes a draw reads changed
   (`gpu_stream_skip_unchanged`). Next: no full guest memory mirror on the GPU and no write
   watching; textures and buffers uploaded when the game loads or changes them (hooks on its
   resource code).
6. Removing the emulation paths, Windows check, Android port.

## Validation

- Offline A/B: `tools/bench/replay_ab.py` replays captured frames through renderer variants
  headlessly, diffs the images and reports GPU time per category. Build the replayer with
  `tools/native-renderer/build_trace_reference.sh`.
- Captures: `tools/bench/autorun.py` drives the game unattended (injected pad input, engine
  screenshots, perf, GPU clock/power) and `trace <label>` grabs a frame trace.
- New GPU-visible code runs on llvmpipe first (`VK_ICD_FILENAMES=.../lvp_icd.x86_64.json`):
  a GPU fault on the Steam Deck resets the GPU and can black out the session. llvmpipe
  replays 1x and 2x traces.
- `TRACE_STREAM_LEARN=1` makes the replayer's memory writes count as write faults, so pages
  the trace rewrites every frame become streamed like in the running game (needs
  `TRACE_BENCH` of 4 or more).
- `REX_CMD_STATS=1` logs the commands, barriers and uploads per command buffer (`=2` also the
  most uploaded ranges, `REX_CMD_STATS_EVERY=<n>` the interval).
- Debug modes that prove exactness: `native_resolve_debug_reload` (textures reload from the
  memory native resolves write), `native_resolve_debug_memory_only_all` (every resolve writes
  only the memory), `native_resolve_debug_verify_stencil_capture`.

## State (2026-10-02)

- Shaders: `aot_export_path` exports runtime translations in the served format;
  `TRACE_SHADER_STORAGE=<cache>:45410809` makes the replayer load a shader storage the way the
  game does at boot; `tools/native-renderer/build_translated_set.sh` builds
  `translated_shaders.tar.xz` (about 1 MB, both scales, with the plain / level 0 texture
  variants), which the release workflow unpacks into `native_shaders/`.
- EDRAM transfers: transfers into render targets cleared right after binding are held back
  over consecutive clears and done only outside the cleared area (the shadow map pass: 0.7 ms
  at 2x down to almost nothing). A draw proves it overwrites render targets entirely either
  with the XDK clear shader (positions read from its vertex buffer) or with any vertex shader
  the CPU interpreter can run (`native_rt_cpu_vs_overwrite_proofs`), which covers the
  full-screen post-processing passes - the effect chains that reinterpret EDRAM at other
  pitches (the super burp's glow) then skip copying dead data. Proofs are checked with
  `native_rt_debug_poison_overwrites`, which fills each proven area with garbage before the draw.
- Clears: the XDK's clear draws are done as clears of the attachments
  (`native_rt_clear_draws_as_clears`) when the draw replaces everything it writes with the
  constants from its vertex buffer, so AMD GPUs fast-clear compressed targets instead of drawing
  every pixel (about 0.45 ms at 2x).
- Post passes: the native post-processing shaders fetch their texture taps in batches (5 or 4
  at a time, the counts this game uses), so the latencies overlap (about 0.45 ms at 2x).
- Stencil: depth resolves of an area whose stencil is known to be uniform (cleared, not
  written since) skip capturing it (`native_resolve_uniform_stencil`, on, about 1 ms at 2x).
- Compression: native resolves write 8_8_8_8, 2_10_10_10 and depth textures through a view of
  their own format (`native_resolve_unorm_views`), so AMD GPUs keep them DCC-compressed like
  the render targets (1.1 to 1.4 ms at 2x).
- GPU time per frame on the Steam Deck replaying captured frames (with the streaming the
  running game does): t09 gameplay about 4.3 ms at 1x and 10.5 ms at 2x, Springfield about
  4.8 ms at 1x and 10.9 ms at 2x (16.7 ms at 2x before the upload and compression work).
  Replays keep the GPU clock low, so only A/B runs made one after the other compare. With the
  batched post passes and native clears, t09 at 2x went from 12.8 to 11.8 ms in such an A/B.
  Live, Springfield at 2x runs at about 59 FPS; the GPU now needs about 1100 MHz there instead
  of 1200 at the same frame rate, which leaves headroom for effects.
- Where 2x GPU time goes (t09, natives): the three post passes about 35% (346B alone 17%),
  resolves about 20% (two depth resolves nothing samples yet about 9%, the post-processing
  chain's color resolves about 6%), world draws most of the rest. Hand-written world shaders
  gain little over their translations (E170: 5%).
