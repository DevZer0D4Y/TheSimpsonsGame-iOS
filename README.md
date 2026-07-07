# The Simpsons Game — Recompiled 🍩

A fan-made **static recompilation** of *The Simpsons Game* (Xbox 360, 2007) to a native
executable — the original game code, translated ahead-of-time from Xenon PowerPC to your CPU,
running against a Xenia-derived GPU/kernel emulation layer. Boots to playable gameplay on
Linux / Steam Deck today.

> **You must own the game.** This repository contains **no game assets, no ISO, and no
> copyrighted content**. The included launcher installs the game from *your own* legally-owned
> Xbox 360 ISO and even generates its artwork from *your* copy's cutscene files.

## Status

| Platform | State |
|---|---|
| Linux / Steam Deck | ✅ Playable (menus, saves, videos, gameplay) |
| Windows | ✅ Builds & boots to gameplay (clang + D3D12) — first bring-up |
| Android | 🗺 Planned |

Input: controller required for now — experimental keyboard/mouse emulation can be enabled in
the launcher (Settings → Input).

## The Launcher

`launcher/` contains a themed desktop launcher (Python + Qt WebEngine, stdlib backend):

- **Install**: point it at your ISO → extracts and installs the game data (bundled
  `extract-xiso`), keeps your previous install as a backup.
- **Play / Stop**, live console output, Add-to-Steam for Gaming Mode.
- **Settings**: resolution presets (720p–4K), render supersampling, FXAA, anisotropic
  filtering, VSync, letterboxing, **framerate (30/60/90/120)**, keyboard/mouse emulation +
  sensitivity, language, audio buffer/mute — written safely into the engine config.
- **Patches**: one-click reversible tweaks (skip intro logos; more coming).
- **Saves**: one-click backup/restore.
- **Updates**: checks this repository's releases.

Run it: `launcher/simpsons-launcher.sh` (a `.desktop` entry is installed automatically).

## Repository layout

```
launcher/            Desktop launcher / installer (Python, zero runtime deps + optional PySide6)
simpsons/            The recompiled game project (generated translation units + CMake build)
tools/rexglue-sdk/   ReXGlue recompilation SDK (runtime, GPU/kernel emulation, codegen builders)
tools/XenonRecomp/   PowerPC→C++ static recompiler (disassembler + emitter)
tools/extract-xiso/  Xbox ISO extraction tool (installer backend)
.github/workflows/   Manual (workflow_dispatch) release workflow — Windows + Linux
```

Not in the repo (see `.gitignore`): game data, prebuilt toolchains (`tools/clang20`,
`tools/rexglue-bin`), build outputs, and anything generated from the player's own game files.

## Building (Linux)

```sh
# 1. SDK runtime
cmake -S tools/rexglue-sdk -B tools/rexglue-sdk/build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DSDL_HIDAPI_LIBUSB=OFF -DSDL_DBUS=OFF
ninja -C tools/rexglue-sdk/build rexruntime

# 2. Game
cmake -S simpsons -B simpsons/out/build/linux -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
ninja -C simpsons/out/build/linux

# 3. Install your game + play
launcher/simpsons-launcher.sh
```

## Building (Windows)

Windows uses the **D3D12** backend and builds with **clang** (not MSVC `cl`).

Prerequisites: LLVM/Clang 20.x, Visual Studio Build Tools with "Desktop
development with C++" (Windows SDK + linker), CMake ≥ 3.25, Ninja, and Python 3
(`pip install PySide6` for the native launcher window). NASM is *not* required —
the vendored FFmpeg is configured with `HAVE_X86ASM=0`.

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

## Known issues

- **"Instant character pop-in" (`gpu_allow_invalid_fetch_constants = true`) is
  dangerous on the Steam Deck's GPU.** It crashes on *any* level load on both
  Linux/Vulkan (RADV) *and* Windows/D3D12 — on Windows it triggers a kernel
  **BSOD** (`PAGE_FAULT_IN_NONPAGED_AREA`, bugcheck `0x50`) in the AMD driver.
  The completely-unmodified D3D12 path fails identically, so this is the Van
  Gogh APU (or the content), **not** a Vulkan/RADV-driver-specific bug as
  previously assumed. Leave it **off** on Deck-class GPUs; it is reported to
  work on desktop GPUs in Xenia.
- Boot logo videos (EA/Fox/Gracie) may flicker green — a bug in the game's own
  guest-side VP6 decoder, independent of the graphics backend.

## ⚠️ Before making this repository public

The `simpsons/generated/` translation units are derived from the game's executable. Before a
public release this repo should switch to the standard recomp-project structure: ship only the
recompiler + configuration, and generate the translation units on the player's machine from
their own copy (the hand-written fixes become a patch overlay applied after generation).
Tracked as a pre-public-release task.

## Credits

- Built on [XenonRecomp](https://github.com/hedge-dev/XenonRecomp)-style static recompilation
  and a [Xenia](https://xenia.jp)-derived emulation runtime (ReXGlue SDK).
- *The Simpsons Game* © 2007 Electronic Arts / Fox. This project is an unaffiliated
  preservation effort and requires an original copy of the game.
