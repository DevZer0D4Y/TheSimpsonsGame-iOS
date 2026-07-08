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
| Windows | ✅ Builds & boots to gameplay (clang + D3D12) — first bring-up, expect rough edges |
| Android | 🗺 Planned |

Input: controller required for now — experimental keyboard/mouse emulation can be enabled in
the launcher (Settings → Input).

## Known issues

- **"Instant character pop-in" patch is dangerous on Steam Deck's GPU.** Characters/props
  normally pop in a few seconds after a level loads (the game's own disc data marks their
  streaming meshes as temporarily "invalid," which the engine correctly declines to draw until
  they're ready — this is the accepted default behavior). There's a known community fix
  (`gpu_allow_invalid_fetch_constants`) that makes them appear instantly instead, exposed in the
  launcher's Patches tab — but on Steam Deck's AMD APU it crashes on *any* level load, on
  **both** Linux/Vulkan (RADV) *and* Windows/D3D12: on Windows it triggers a kernel **BSOD**
  (`PAGE_FAULT_IN_NONPAGED_AREA`, bugcheck `0x50`) in the AMD driver. The same crash on two
  independent, completely unrelated driver stacks (and the D3D12 code path here is entirely
  unmodified, stock Xenia logic) points at the Van Gogh APU itself, not a driver or engine bug.
  It's reported to work on most other/desktop GPUs. **Leave it off on Steam Deck** unless you're
  specifically testing it.
- Boot logo videos (EA/Fox/Gracie Films) may show green flicker — a bug in the *game's own*
  guest-side video decoder, not something introduced by this port.
- In-game audio can sound crunchy under load; main menu UI may flicker slightly.

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

## Credits

- Built on a [XenonRecomp](https://github.com/hedge-dev/XenonRecomp)-style static
  recompilation approach and a [Xenia](https://xenia.jp)-derived emulation runtime (ReXGlue
  SDK).
- Bundles/links SDL3, Dear ImGui, glslang, SPIRV-Tools, FFmpeg, spdlog, fmt, and the Tracy
  profiler — each under their own respective open-source licenses; see
  `tools/rexglue-sdk/thirdparty/` for each project's source and license.
- *The Simpsons Game* © 2007 Electronic Arts / Fox. This project is an unaffiliated
  preservation effort and requires an original copy of the game.
