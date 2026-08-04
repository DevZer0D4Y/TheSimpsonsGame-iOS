# The Simpsons Game — Recompiled 🍩

A fan-made **static recompilation** of *The Simpsons Game* (Xbox 360, 2007) to a native
executable — the original game code, translated ahead-of-time from Xenon PowerPC to your CPU,
running against a Xenia-derived GPU/kernel emulation layer.

> **You must own the game.** This repository contains **no game assets, no ISO, and no
> copyrighted content**. The included launcher installs the game from *your own* legally-owned
> Xbox 360 ISO and even generates its artwork from *your* copy's cutscene files. See
> [Legal & disclaimers](#legal--disclaimers) below.

## Download & Play

The easiest way to play — no build tools required:

1. Grab the latest release for your OS from the
   [Releases page](https://github.com/YesterMester/TheSimpsonsGameRecomp/releases).
2. Extract the zip anywhere.
3. **Linux/Steam Deck:** run `Play.sh` at the top of the extracted folder.
   **Windows:** run `simpsons-launcher.exe` at the top of the extracted folder — it's a
   standalone app, no Python install required.
4. In the launcher's **Install** tab, point it at your own Simpsons Game Xbox 360 ISO. It
   extracts and installs the game data for you.
5. Hit **Play**.

The launcher checks for and installs new releases itself (About tab → Check for updates), so
you don't need to manually re-download after the first time.

> **Steam Deck players:** read the pop-in-patch warning in the launcher's Patches tab before
> touching the "Instant character pop-in" toggle — it's known to hang the GPU on Steam Deck's
> hardware, on both SteamOS and Windows. See [Known issues](#known-issues).

## Status

| Platform | State |
|---|---|
| Linux / Steam Deck | ✅ Playable (menus, saves, videos, gameplay) |
| Windows | ✅ Builds & boots to gameplay (clang + D3D12) — first bring-up, expect rough edges. So far only actually tested on a Steam Deck's Windows dual-boot partition (Van Gogh APU); not yet verified on a general desktop PC / discrete GPU. |
| Android | 🗺 Planned |

Input: controller required for now — experimental keyboard/mouse emulation can be enabled in
the launcher (Settings → Input).

## Requirements

- **Your own legally-owned Xbox 360 copy of The Simpsons Game**, as an ISO you supply yourself.
  Primarily tested against the USA release (title ID `45410809`); the EUR release has also been
  confirmed to boot and play, though it's had less overall testing.
- **~5 GB free disk space** for the extracted game data (movies, audio, levels — roughly 4 GB
  once extracted), on top of the launcher/engine download itself.
- A GPU with reasonably current Vulkan (Linux) or D3D12 (Windows) driver support. Integrated/APU
  graphics (Steam Deck included) work, see the pop-in patch caveat below for the one known
  exception.
- A controller is currently required for gameplay (see above).

## Known issues

- **Missing characters ("eternal pop-in") — largely fixed by default.** The game marks the
  vertex streams of streamed meshes "invalid" while they load, and additionally leaves the
  *optional* blend-shape streams of many finished character meshes permanently marked invalid.
  The engine used to throw away any draw touching an invalid stream, which is why characters
  could stay invisible indefinitely. It now tells the two cases apart: meshes whose only
  "invalid" streams are those absent optional ones (which their shaders provably never read)
  are drawn normally, by default, on every platform (`gpu_allow_null_optional_streams`).
  Entities genuinely mid-stream still appear a moment later — that's real loading. If you still
  see long-lasting invisible characters, grab the log and open an issue; the old Level-1
  camera-into-wall workaround should no longer be necessary.
- **"Instant character pop-in" patch is dangerous on Steam Deck's GPU.** On top of the default
  fix above, the community's full fix (`gpu_allow_invalid_fetch_constants`, launcher Patches
  tab) also runs the game's stale-descriptor streaming "priming" draws — but on Steam Deck's
  AMD APU it has crashed on level loads, on **both** Linux/Vulkan (RADV) *and* Windows/D3D12:
  on Windows it triggers a kernel **BSOD** (`PAGE_FAULT_IN_NONPAGED_AREA`, bugcheck `0x50`) in
  the AMD driver. The same crash on two independent driver stacks points at the Van Gogh APU
  itself. Those priming draws now run with rasterization disabled entirely (they only exist to
  feed the game's vertex-readback streaming), which is expected to remove the crash trigger,
  but this hasn't been re-verified on Deck hardware yet. **Leave it off on Steam Deck** unless
  you're specifically testing it.
- **Windows saves moved to the right place.** Saves and the shader cache used to be written to
  your **Documents** folder, which is wrong for Windows and breaks badly when Documents is
  redirected into OneDrive. They now live in `%LOCALAPPDATA%\simpsons` (the local app data
  folder). If you already had saves in Documents, the engine moves them across automatically
  the first time you launch it and notes the move in the log — you don't need to do anything.
  The disposable shader cache is not moved; it just rebuilds. Linux is unchanged
  (`~/.local/share/simpsons`).
- Boot logo videos (EA/Fox/Gracie Films) may show green flicker — a bug in the *game's own*
  guest-side video decoder, not something introduced by this port. (THIS SHOULD BE FIXED)
- In-game audio can sound crunchy under load.
- **Main menu flicker / UI elements flying around.** Two separate bugs wore this costume:
  1. *Tearing.* The Vulkan presenter defaulted to tearing-permitted ("immediate") presentation.
     It now prefers a tear-free mode (mailbox), and on drivers that don't offer mailbox the
     launcher's VSync setting (on by default) rules out the tearing-permitted fallbacks too, so
     presentation lands on classic vsync instead of silently tearing. The engine log's
     "Created … swapchain" line names the chosen mode in plain words if you want to check.
  2. *Menu elements jumping to wrong positions while navigating.* This one wasn't presentation
     at all — it was a cache-coherence race in the engine's guest-memory mirror. The table
     tracking which memory pages still need re-uploading to the GPU was double-buffered and
     swapped once per frame *without* the lock every other writer holds, so a page the game had
     just rewritten could keep a stale "already uploaded" mark. The engine then skipped that
     upload and the GPU drew the previous frame's vertex data — scattered UI quads landing at
     last frame's positions for a frame or two, which is why it showed up while moving through
     the menu and not while sitting still. The page table is now a single lock-protected copy
     (matching upstream Xenia's semantics), which cannot go stale. Please report whether this
     clears it up.
- The framerate setting (Settings → Framerate) applies the same guest instruction patch as the
  community's own 60 FPS unlock for this game (verified against the `xenia-canary/game-patches`
  entry for this title) on top of reporting a higher display refresh rate — both were required;
  reporting the higher refresh rate alone did nothing; the game had its own hardcoded "wait 2
  vblanks between frames" that needed patching out. 60 removes that hardcoded halving cleanly.
  A companion patch the community version also applies (compensating timing used elsewhere in
  the game, said to avoid clipping/collision/animation artifacts) targets guest code outside
  anything this project's recompiler discovered as a function, so it isn't ported yet — watch
  for occasional clipping or animation oddities at 60+, especially 90/120 (experimental: the
  *game's own* scripted/physics timing wasn't designed for those rates). Actual achieved
  framerate still depends on real performance in the current scene — expect it to track close to
  your target in light scenes and fall short in heavier ones on Steam Deck's APU; this is an
  active area of work, not a hard cap anymore.

## Performance roadmap

Where this project is headed, in plain language.

**The big picture:** today the recompiled game code runs natively on your CPU, but everything
it asks the GPU to do still goes through an emulation-style translation layer (derived from the
Xenia emulator): the game writes Xbox 360 GPU command packets, and the engine decodes them,
emulates the 360's unusual video memory (EDRAM), and translates its shaders to Vulkan/D3D12 at
runtime. That layer is mature and correct — but it's also where most of the remaining
performance and smoothness is left on the table. The long-term goal is to **progressively
replace that layer with a purpose-built native renderer** for this specific game, the same
strategy other successful recompilation projects have used. Each stage below is useful on its
own, and the current renderer always remains as a working fallback.

### ✅ Recently shipped

- **Missing characters drawn by default**: finished character meshes whose absent optional
  blend-shape streams are permanently marked "invalid" on disc are no longer thrown away —
  they draw normally out of the box, without the crash-prone experimental pop-in patch (see
  Known issues).
- **Guaranteed tear-free presentation**: with VSync on, the presenter can no longer silently
  fall back to a tearing-permitted mode on drivers without mailbox support.
- **Fixed stale-geometry corruption** (menu elements flying to wrong positions): the
  valid-page table behind the guest-memory→GPU mirror was swapped between two buffers each
  frame without holding the lock its other writers use, so an invalidation could be lost and a
  rewritten page stay marked "already uploaded" — the GPU then drew last frame's vertices.
  Collapsed to one lock-protected table. A matching interval-overlap bug in the converted-index
  cache (which skipped invalidating any entry a write fully covered) was fixed alongside it.
- **GPU thread no longer burns a core while idle**: the command processor used to spin-yield
  aggressively whenever the ring drained, competing with the recompiled game code for CPU; it
  now naps on its wake event (microsecond wake-up) after a short spin. Guest-side GPU waits
  also poll finer-grained (500 µs instead of up to several ms), and the CPU-ahead-of-GPU stall
  waits on a single fence instead of every in-flight one.
- **Real 60 FPS**: the game hardcoded "wait 2 screen refreshes between frames" (a 30 FPS lock)
  independent of any settings — found and patched at the instruction level, matching the
  community's verified 60 FPS patch for this title. The Framerate setting now actually works.
- **Tear-free presentation**: main-menu flicker traced to a tearing-permitted swapchain default;
  now prefers a tear-free mode with the same latency.
- **Engine hot-path fixes**: removed a per-draw-call index-cache that the code's own upstream
  history documents as a measured performance loss (it ran on *every* draw, and its per-frame
  cache could never help unique draws); eliminated redundant re-acquisitions of the engine's
  global lock on two hot paths (guest thread-sync calls, and the memory-fault handler that fires
  tens of thousands of times during level streaming).
- **Full-speed builds**: the 82,000 recompiled game functions — where nearly all CPU time goes —
  were compiling at a lower optimization level than the rest of the project, and local builds
  lacked the modern-CPU instruction baseline releases already used. Now unified: `-O3`,
  `x86-64-v3` (AVX2-era, same requirement releases already had), and optional ThinLTO that
  optimizes across all translation units at once.
- **Built-in frame profiling**: the engine now logs per-frame stats (FPS, frame time, draw
  calls, GPU sync stalls, cache hit rates) to a CSV when `perf_log_csv` is set in the config —
  every optimization on this roadmap gets measured, not guessed.

### 🔬 Phase 1 — measure & squeeze (in progress)

- **Profile-guided optimization**: build once with instrumentation, play a real session, rebuild
  — the compiler then lays out those 82k functions using real hotness data instead of guesses.
- **Real gameplay captures**: current data covers the main menu (which issues a startling
  ~3,700 tiny draw calls per frame at ~3 vertices each — likely per-glyph text/UI); in-level
  profiles will decide what gets optimized next.
- **Find the draw-burst source**: identify exactly which recompiled game function issues that
  draw storm, so it can be fixed at the source rather than worked around downstream.

### 🔧 Phase 2 — targeted engine surgery

- **Batch the UI draw burst**: hook the specific game routine responsible (the recompiled code
  supports overriding individual functions) and submit its glyphs/quads as a handful of draws
  instead of thousands.
- **Shader-compile stutter policy**: when new shader effects appear (scene transitions), the
  engine currently holds back whole frames while compiling — tune/expose this tradeoff.
- **Pipeline pre-warming**: optionally build the shader/pipeline cache right after install
  instead of during your first play session.

### 🚀 Phase 3 — the native renderer ("de-emulating" the GPU)

The end-game, done as incremental takeovers rather than a risky rewrite:

1. **Inventory**: dump and count the game's unique shaders (likely a few hundred — small enough
   to convert ahead-of-time, offline) and map the boundary where "the game decides what to draw"
   turns into GPU command packets.
2. **Take over presentation**: replace the swap/present path first — small, provable, reversible.
3. **Take over UI/2D**: a slim native batcher for menu/HUD/text rendering — this alone
   eliminates the draw-storm problem at its source.
4. **Take over world rendering**: the game's shaders converted ahead-of-time, its render passes
   implemented directly, no more runtime command-packet decoding or EDRAM emulation on the hot
   path — with the current renderer kept compilable as a reference/fallback throughout.

## Reporting issues

Found a bug? Please [open an issue](https://github.com/YesterMester/TheSimpsonsGameRecomp/issues)
— include your platform, GPU, what you were doing, and (if the game crashed) whatever log lines
the launcher's Play tab console shows. Check the known issues above first in case it's already
a documented limitation rather than a new bug.

## The Launcher

`launcher/` contains a themed desktop launcher (Python + Qt WebEngine, stdlib backend; ships as
a standalone `.exe` on Windows, no Python required):

- **Install**: point it at your ISO → extracts and installs the game data (bundled
  `extract-xiso`), keeps your previous install as a backup.
- **Play / Stop**, live console output, Add-to-Steam for Gaming Mode.
- **Settings**: resolution presets (720p–4K), render supersampling, FXAA, anisotropic
  filtering, VSync, letterboxing, **framerate (30/60/90/120)**, keyboard/mouse emulation +
  sensitivity, language, audio buffer/mute — written safely into the engine config.
- **Patches**: one-click reversible tweaks (skip intro logos, experimental pop-in fix; more
  coming).
- **Saves**: one-click backup/restore, with automatic self-healing of saves left truncated by
  a crash.
- **Updates**: checks this repository's GitHub releases and can download + install the latest
  one in place (About tab). It only ever replaces engine/launcher-code files — your settings,
  save backups, and mods are never touched by an update.

Running from source: `launcher/simpsons-launcher.sh` (Linux) or `python launcher/launcher.py`
(Windows, needs Python 3 + `pip install PySide6` for the native window — falls back to opening
in your browser otherwise). A `.desktop` entry is installed automatically on Linux.

## Repository layout

```
launcher/            Desktop launcher / installer (Python, zero runtime deps + optional PySide6)
simpsons/            The recompiled game project (generated translation units + CMake build)
tools/rexglue-sdk/   ReXGlue recompilation SDK (runtime, GPU/kernel emulation, codegen builders)
tools/XenonRecomp/   PowerPC→C++ static recompiler (disassembler + emitter)
tools/extract-xiso/  Xbox ISO extraction tool (installer backend)
.github/workflows/   build.yml: compile check on every push. release.yml: manual, produces the
                      packaged per-platform downloads on the Releases page.
```

Not in the repo (see `.gitignore`): game data, prebuilt toolchains (`tools/clang20`,
`tools/rexglue-bin`), build outputs, and anything generated from the player's own game files.

## Building from source (Linux)

Needs: Clang 20, CMake ≥ 3.25, Ninja, and (for the SDK's UI layer) `libgtk-3-dev`,
`libx11-xcb-dev`, and SDL3's usual X11/Wayland/audio dev packages (see `.github/workflows/
build.yml` for the exact `apt` list CI uses).

```sh
# One unified configure: builds the ReXGlue SDK and the game together.
cmake -S simpsons -B simpsons/out/build/linux -G Ninja \
      -DCMAKE_BUILD_TYPE=RelWithDebInfo \
      -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
      -DREXSDK_DIR="$(pwd)/tools/rexglue-sdk"
ninja -C simpsons/out/build/linux

# Install your game + play
launcher/simpsons-launcher.sh
```

`REXSDK_DIR` points the game project straight at the SDK's source tree (see
`simpsons/generated/rexglue.cmake`), so there's no separate SDK install step. This is the same
approach both CI workflows use.

## Building from source (Windows)

Windows uses the **D3D12** backend and builds with **clang** (not MSVC `cl`).

Prerequisites: LLVM/Clang 20.x, Visual Studio Build Tools with "Desktop development with C++"
(Windows SDK + linker), CMake ≥ 3.25, Ninja, and Python 3 (`pip install PySide6` for the native
launcher window). NASM is *not* required — the vendored FFmpeg is configured with
`HAVE_X86ASM=0`.

```powershell
# Build the game + SDK together (points the game at the SDK source tree):
cmake --preset win-amd64-relwithdebinfo `
      -DREXSDK_DIR="$PWD/tools/rexglue-sdk" `
      "-DCMAKE_C_FLAGS=-march=x86-64-v3" "-DCMAKE_CXX_FLAGS=-march=x86-64-v3"
cmake --build --preset win-amd64-relwithdebinfo --target simpsons

# ISO extraction tool (WIN32 must be defined for the vendored getopt):
cmake -S tools/extract-xiso -B tools/extract-xiso/build -G Ninja `
      -DCMAKE_C_COMPILER=clang "-DCMAKE_C_FLAGS=-DWIN32"
cmake --build tools/extract-xiso/build

# Install your game + play
python launcher/launcher.py
```

The build stages `rexruntime*.dll` and `TracyClient*.dll` next to `simpsons.exe`
automatically — Windows has no `LD_LIBRARY_PATH`, so DLLs live beside the exe.

### Building with Vulkan on Windows

Windows defaults to the **D3D12** backend, but the **Vulkan** backend also builds and
runs on Windows (it is the default on Linux). This is useful if you want the fragment
shader interlock render path, or if you are hitting a D3D12-only issue — for example,
FMV playback (`.vp6`) currently shows a black screen on the host render target path
(D3D12, and Vulkan with `render_target_path_vulkan=""`), while it plays fine on the
Vulkan interlock path.

Prerequisites are the same as the D3D12 build above. Configure a separate build
directory so the SDK output does not clobber the D3D12 one:

```powershell
cmake -S simpsons -B simpsons/out/build/win-vk -G Ninja `
      -DCMAKE_BUILD_TYPE=RelWithDebInfo `
      -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ `
      "-DCMAKE_CXX_FLAGS=-march=x86-64-v3" `
      -DREXGLUE_USE_D3D12=OFF -DREXGLUE_USE_VULKAN=ON `
      -DREXSDK_DIR="$PWD/tools/rexglue-sdk"
cmake --build simpsons/out/build/win-vk --target simpsons
```

Notes:

- The Vulkan backend links `dxgi` on Windows because the presenter (`rexui`) uses
  DXGI for the swapchain regardless of the graphics backend (this was only linked in
  the D3D12 branch).
- The SDK output directory (`tools/rexglue-sdk/out/win-amd64`) is shared between the
  D3D12 and Vulkan builds — building the Vulkan variant overwrites
  `rexruntimerd.dll` there, so back it up before switching back to D3D12.
- Use a `RelWithDebInfo`/`Release` build type: without `-DNDEBUG`, the render path
  assertions (`GetPath() == Path::kHostRenderTargets`) fire on the interlock path.

## Legal & disclaimers

*The Simpsons Game* © 2007 Electronic Arts / Fox. This is an unaffiliated, non-commercial
preservation/compatibility project — it is **not** endorsed by, sponsored by, or affiliated
with Electronic Arts, Fox, Microsoft, or any rightsholder. Xbox 360 and related marks are
trademarks of their respective owners.

This repository and its releases contain **zero original game assets, audio, video, or
copyrighted content of any kind**. `simpsons/generated/` contains hand-corrected, machine-
translated C++ produced by statically recompiling the game's executable code — no game data
(textures, models, audio, video, script/level data) is included or redistributed anywhere in
this repo or its build output. Everything gameplay-related is read at runtime, locally, from
an ISO **you** supply from **your own** legally-owned copy of the game. Nothing you install is
ever uploaded anywhere.

You are responsible for complying with the laws that apply to you regarding backing up or
format-shifting media you own. This software is provided with **no warranty of any kind**;
use it at your own risk.

This project's own original code (launcher, hand-written engine patches, build tooling) is
licensed under the [GNU GPLv3](LICENSE). Vendored/bundled third-party components keep their own
original licenses — see [Credits](#credits) below and `tools/rexglue-sdk/thirdparty/`.

**A note on AI assistance:** parts of this project — research into the game's internals, build
tooling, and bug fixes — have been developed with the assistance of Claude AI, in the interest of
getting a working release out and turning around fixes as quickly as possible for a solo,
fan-made effort. Treat it as a fast-moving hobby project rather than a polished commercial
release.

## Credits

- Built on a [XenonRecomp](https://github.com/hedge-dev/XenonRecomp)-style static
  recompilation approach and a [Xenia](https://xenia.jp)-derived emulation runtime (ReXGlue
  SDK).
- Bundles/links SDL3, Dear ImGui, glslang, SPIRV-Tools, FFmpeg, spdlog, fmt, and the Tracy
  profiler — each under their own respective open-source licenses; see
  `tools/rexglue-sdk/thirdparty/` for each project's source and license.
- *The Simpsons Game* © 2007 Electronic Arts / Fox. This project is an unaffiliated
  preservation effort and requires an original copy of the game.
