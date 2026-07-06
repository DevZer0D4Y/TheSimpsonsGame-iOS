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
| Windows | 🚧 In development — built by CI (`.github/workflows/build.yml`) |
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
.github/workflows/   CI: Linux build + experimental Windows build
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
