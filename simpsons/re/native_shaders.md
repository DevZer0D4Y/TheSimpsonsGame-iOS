# Native shaders - taking the GPU shader translation offline

Status note for the "de-emulate the GPU" work, shader half. This is the part
with the clearest analogy to what the project already did for the CPU: instead
of translating shaders at runtime, translate them ahead of time. As of this
writing the offline translation itself works end to end on the game's full
shader set; what remains is having the engine consume the pre-translated
results on the hot path. Details and honest status below.

## What a Xenos shader is here, and how it's translated today

The game's shaders are Xbox 360 GPU (Xenos) microcode. The engine (a Xenia
fork, in `tools/rexglue-sdk/src/graphics/pipeline/shader/`) translates each one
to SPIR-V for Vulkan (or DXBC for D3D12) the first time it's needed during play,
then caches the result. That runtime translation is exactly the kind of work
static recompilation removed from the CPU side. The goal here is to do it once,
offline.

## The corpus is small and already on disk

The engine writes every unique shader it translates into a cache file,
`<title_id>.xsh` under `cache/shaders/shareable`, keyed by an XXH3 hash of the
microcode. That means the set of shaders the game uses is already enumerated on
disk from normal play - no need to re-run the game to collect it, and no
guessing at the count.

`tools/shaderkit/xsh_inventory.py` reads that file. On a real cache from actual
play sessions it reported:

    unique shaders:   153   (93 vertex, 60 pixel)
    microcode total:  77,208 bytes
    per-shader size:  36 - 1668 bytes

So the README's "likely a few hundred" estimate holds, with real numbers. The
count grows as more of the game is played and more shaders get cached; 153 is
what a partial playthrough had recorded. The full set is still small enough to
translate exhaustively offline.

### The `.xsh` format (read from the engine's own writer)

Little-endian. From `rexglue-sdk` vulkan/pipeline_cache.h, `ShaderStoredHeader`:

    file header:  u32 magic 'XESH' (0x48534558), u32 version (byte-swapped 0x20201219)
    then repeated:
      u64 ucode_data_hash               XXH3 of the microcode
      u32 packed:  bits 0..30 = dword count,  bit 31 = type (0 vertex, 1 pixel)
      <dword_count * 4> bytes of microcode

`xsh_inventory.py` parses this and can extract each shader's microcode to a file
named `<hash>.<vs|ps>.ucode`.

Cross-check that the parser is right: the hashes it reads match the ones the
engine logs at pipeline-creation time. The engine logged
`VS 0A6D1DD7767FDF27, PS 2E372EA28CC404B7`; the tool independently read those
same hashes with the same vertex/pixel types out of the cache. So the inventory
lines up with what the engine actually binds.

## Offline translation works

`tools/shaderkit/xenos_translate` is the offline translator. It does not
reimplement anything - it links the engine's runtime and calls the same
`Shader::AnalyzeUcode` -> `TranslateAnalyzedShader` path the engine uses
internally, so its SPIR-V comes from the exact code that renders the game.

Result on the full extracted corpus:

    153 / 153 shaders translated to valid SPIR-V
    0 failures, 0 malformed outputs
    output: SPIR-V 1.5, well-formed headers

So the hard part - turning the game's actual shaders into native shader code
ahead of time - is done and works across the whole set.

### The one real gotcha: ABI skew

The translator links the engine runtime, so it must be built against the same
source tree the runtime was built from. Building against the prebuilt SDK's
shipped headers while linking a runtime `.so` from a different revision gives
the shader-translator structs a different layout than the library expects, and
it crashes (a null-ish `this`, a bad `unique_ptr` reset inside
`SpirvShaderTranslator::Reset`). Building against the SDK source headers and
linking the source-built runtime - one tree, no skew - fixes it. That's what
`tools/shaderkit/build.sh` does, and why it doesn't use CMake `find_package`
against the prebuilt SDK.

The compile definitions also have to match the SDK's own build
(`REXGLUE_ENABLE_PROFILING`, `REXGLUE_ENABLE_PERF_COUNTERS`, `REX_HAS_VULKAN`,
the Tracy set) because they change struct sizes. build.sh sets them.

## What's done, and what's left

Done, and verified:

- Enumerate and extract the game's real shader set from the on-disk cache
  (`xsh_inventory.py`), cross-checked against the engine's own logged hashes.
- Translate every one of them to valid SPIR-V offline, using the engine's own
  translator (`xenos_translate`, 153/153).

Left - the integration that makes it actually "native":

- **Bake step.** Run the whole corpus through the translator and emit a
  hash-keyed table (a generated header or a data blob) mapping each shader hash
  to its pre-translated SPIR-V. This is straightforward now that single-shader
  translation works; it's a loop plus a codegen step.
- **Engine consume step.** Have the pipeline cache, on a cache miss, look the
  shader up in the baked table by hash and use the pre-translated SPIR-V instead
  of calling the runtime translator. This is the change that removes the runtime
  cost. It's a targeted edit in `vulkan/pipeline_cache.cpp` (and the D3D12
  equivalent), behind a flag, with the runtime translator kept as the fallback
  for any shader not in the table.
- **Modifications.** The translator emits a canonical translation per shader.
  The engine sometimes needs shader variants ("modifications" - fixed-function
  state folded into the shader). A complete bake would enumerate the
  modifications the game's pipelines actually use (they're recorded in the
  `.xpso` pipeline cache next to the `.xsh`) and translate each. The current
  tool does the default modification; wiring in the full set is part of the bake
  step.

None of the remaining work needs the runtime translator to be reverse-
engineered or reimplemented - it's already being reused directly. It's bake +
consume + variants, all tractable, all incremental, with the runtime path intact
as a fallback throughout.

## DXBC / D3D12

Everything above is the Vulkan/SPIR-V path. The same translator has a DXBC
backend (`dxbc_translator*`) for the D3D12/Windows build; `xenos_translate`
currently targets SPIR-V. A DXBC mode is the same shape of work against the
already-present code.
