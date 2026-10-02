# The Simpsons Game Recompiled

A native PC port of *The Simpsons Game* (Xbox 360, 2007) for Linux, Steam Deck and Windows, made by
statically recompiling the original game code.

The game's PowerPC executable is translated ahead of time into C++ and compiled for x86-64, so the
game's own code runs directly on your CPU rather than inside an emulator. Underneath it, the
ReXGlue runtime, which is derived from the Xenia project, provides the Xbox 360 kernel, audio, input
and graphics layers, with a Vulkan renderer on Linux and Direct3D 12 or Vulkan on Windows.

**This project does not include any game content. You need your own copy of the Xbox 360 game.**

[Download the latest release](https://github.com/YesterMester/TheSimpsonsGameRecomp/releases/latest)

## Contents

- [Status](#status)
- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Configuration](#configuration)
- [Known issues](#known-issues)
- [Building from source](#building-from-source)
- [Contributing](#contributing)
- [Reporting bugs](#reporting-bugs)
- [Project layout](#project-layout)
- [Roadmap](#roadmap)
- [Legal](#legal)
- [License](#license)
- [Credits](#credits)

## Status

| Platform | Status |
|---|---|
| Linux (x86-64) | Playable |
| Steam Deck | Playable, and the main test platform |
| Windows (x86-64) | Playable, less tested than Linux, mostly on Steam Deck hardware |
| Android | Planned |

The game boots, plays its videos, saves and loads, and runs its levels. See
[Known issues](#known-issues) for what still misbehaves.

## Features

- Native x86-64 executable: the recompiled game code runs without CPU emulation.
- Vulkan renderer that draws with the GPU's own render targets, plus an accurate fallback path
  that emulates the Xbox 360's EDRAM in the pixel shader.
- 60 FPS gameplay, with menus, the title screen and loading screens kept at the original 30 FPS so
  they run at the speed they were made for.
- Render resolution scaling (supersampling), anisotropic filtering and FXAA.
- A launcher that installs the game from your ISO, manages settings, patches and save backups,
  adds the game to Steam, and updates itself.
- Controller support, and keyboard and mouse controls with rebindable keys and an in-game list of
  the controls.

## Requirements

- Your own copy of *The Simpsons Game* for Xbox 360, as an ISO image. The USA release (title ID
  45410809) is the main target. The European release also boots and plays but has seen less
  testing.
- About 5 GB of free disk space for the installed game data.
- A 64-bit x86 CPU. The standard download needs AVX2 (Intel Haswell, AMD Excavator or Zen, or
  newer, roughly 2013 onwards). For older CPUs with SSE4.2, download the package ending in
  `-NoAVX2` instead; it runs the same game, somewhat slower.
- A GPU with a current Vulkan driver (Linux) or Direct3D 12 driver (Windows).
- A controller is recommended.

## Installation

1. Download the archive for your platform from the
   [Releases page](https://github.com/YesterMester/TheSimpsonsGameRecomp/releases). Use the
   `-NoAVX2` package if your CPU predates AVX2 (the standard package then fails to start, on
   Windows with error 0xc0000142).
2. Extract it anywhere.
3. Start the launcher:
   - Linux and Steam Deck: run `Play.sh`.
   - Windows: run `simpsons-launcher.exe`. It is a standalone program and does not need Python.
4. In the **Install** tab, select your Xbox 360 ISO. The launcher extracts and installs the game
   data.
5. Press **Play**.

On Steam Deck, use **Add to Steam** in the launcher to play from Game Mode.

To start the game without the launcher window, for example from your own Steam shortcut, run
`Play.sh --play` on Linux or `simpsons-launcher.exe --play` on Windows. It uses the same settings
as the launcher's Play button.

The launcher can update itself from the **About** tab. Updates replace only the engine and
launcher files. Your settings, saves and mods are kept. If the installed package does not match
your CPU, the launcher offers the right one.

**Antivirus warnings.** The Windows programs in the release are not code-signed, so Microsoft
Defender and browsers such as Edge sometimes flag or block the download, occasionally with a
generic detection like `Trojan:Win32/Suschil!rfn`. These are false positives: every release is
built from this repository's source by the public GitHub Actions workflow in
`.github/workflows/release.yml`. If your download is blocked, you can build the game yourself
(see [Building from source](#building-from-source)) or restore the file from Defender's
protection history.

## Configuration

Most settings are in the launcher's **Settings** tab, saved as soon as you change them. They are
written to `simpsons.toml` next to the game executable, inside a block the launcher manages.
Settings outside that block can be edited by hand. The launcher has a light and a dark theme; the
button at its top right switches between them.

Saves and the shader cache are stored in:

| Linux | Windows |
|---|---|
| `~/.local/share/simpsons` | `%LOCALAPPDATA%\simpsons` |

**Frame rate.** 60 is the default and the recommended setting. 30 matches the original console
exactly. The game's frame scheduler cannot go above 60 FPS, so the 90 and 120 settings do not add
frames; they make frame pacing less even. Menus and loading screens run at 30 FPS regardless,
because the game runs their logic once per frame (`menu_frame_rate` in `simpsons.toml`, 0 turns
this off).

**Image quality.** FXAA anti-aliasing is on by default for new installs and smooths the
cel-shading outlines at little cost. A render scale of 2x or 3x supersamples the whole image, which
gives the outlines their cleanest look. On a Steam Deck, 1x holds 60 FPS; 2x looks much sharper
and runs at about 59 FPS in Springfield, the busiest area, with occasional drops.

**Keyboard and mouse.** Turn on *Play with keyboard & mouse* in the launcher's Settings tab. The
game then takes the mouse whenever its window is active, and lets go of it when you switch to
another window. The game's own prompts show controller buttons; press F1 in the game to see which
key each button is on. The default layout:

| Action | Keys |
|---|---|
| Move | W A S D |
| Look around | Mouse |
| Jump, confirm | Space, Enter |
| Attack | Left click |
| Special attack (hold), back | Right click, Backspace |
| Action: talk, use, pick up | E |
| Target (hold) | Shift |
| Walk (hold) | Ctrl |
| Switch character | 1 to 4, arrow keys |
| To-do list | Tab |
| Pause | Esc |

Every control can be changed in the launcher's **Keyboard & mouse** section, where an action can
have several keys, mouse buttons included, and the wheel and side buttons can be bound. The camera
can also be put on keys (Look up, down, left and right), on top of the mouse. In the
game, F4 opens the settings, with the controls under Input > Keybinds (press *Save to config* to
keep changes made there).

## Known issues

- At 60 FPS some scripted sequences can misbehave, because the game was built for 30 FPS. A known
  case is the dam in "Lisa the Tree Hugger", where random deaths can happen at 60; switch to 30
  for that section if it happens.
- If videos show a black screen on Windows, switch the graphics backend to Vulkan in the
  launcher's settings.

## Building from source

The game and the ReXGlue SDK are built together in one CMake project. Continuous integration uses
exactly these steps; see `.github/workflows/build.yml` for the full list of Linux packages.

### Linux

Requirements: Clang 20, CMake 3.25 or newer, Ninja, pkg-config, and the development packages for
GTK 3, Vulkan, X11 and XCB, Wayland, xkbcommon, udev, ALSA, PulseAudio and PipeWire.

```sh
git clone https://github.com/YesterMester/TheSimpsonsGameRecomp.git
cd TheSimpsonsGameRecomp/simpsons
cmake --preset linux-amd64-relwithdebinfo \
      -DREXSDK_DIR="$PWD/../tools/rexglue-sdk" \
      "-DCMAKE_C_FLAGS=-march=x86-64-v3" "-DCMAKE_CXX_FLAGS=-march=x86-64-v3"
cmake --build --preset linux-amd64-relwithdebinfo --target simpsons
cd ..

# ISO extraction tool used by the launcher's installer
cmake -S tools/extract-xiso -B tools/extract-xiso/build -G Ninja -DCMAKE_C_COMPILER=clang
cmake --build tools/extract-xiso/build

# Install your game and play
launcher/simpsons-launcher.sh
```

The Linux preset calls the compiler `clang-20`. If your distribution names it `clang`, add
`-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++` to the first `cmake` command.

### Windows

Requirements: LLVM/Clang 20, Visual Studio Build Tools with the "Desktop development with C++"
workload (for the Windows SDK and linker), CMake 3.25 or newer, Ninja and Python 3.

```powershell
git clone https://github.com/YesterMester/TheSimpsonsGameRecomp.git
cd TheSimpsonsGameRecomp\simpsons
cmake --preset win-amd64-relwithdebinfo `
      -DREXSDK_DIR="$PWD/../tools/rexglue-sdk" `
      "-DCMAKE_C_FLAGS=-march=x86-64-v3" "-DCMAKE_CXX_FLAGS=-march=x86-64-v3"
cmake --build --preset win-amd64-relwithdebinfo --target simpsons
cd ..

# ISO extraction tool (WIN32 must be defined for the bundled getopt)
cmake -S tools/extract-xiso -B tools/extract-xiso/build -G Ninja `
      -DCMAKE_C_COMPILER=clang "-DCMAKE_C_FLAGS=-DWIN32"
cmake --build tools/extract-xiso/build

# Install your game and play
pip install PySide6
python launcher\launcher.py
```

Windows builds include both the Direct3D 12 and Vulkan renderers; the launcher's settings choose
between them. Without PySide6 the launcher opens in your web browser instead of its own window.

## Contributing

Contributions are welcome, from bug reports to fixes and new features.

**Before you start**

- For anything larger than a small fix, open an issue first to discuss the approach.
- Check the open issues and pull requests so work is not duplicated.

**Pull requests**

- Branch from `main` and open the pull request against `main`.
- Keep each pull request to one change. Describe what it fixes or adds and how you tested it:
  platform, GPU, and what you played.
- The Build check workflow must pass. It builds the game on Linux and Windows for every pull
  request.
- Follow the style of the code around your change. The SDK includes a `.clang-format`.
- To change how the game behaves, prefer overriding a recompiled function from `simpsons/src` over
  editing files in `simpsons/generated`, which the recompiler produces.
  `simpsons/src/frame_pacing.cpp` and `simpsons/src/subtitles.cpp` show how.
- Launcher settings are stored in `simpsons.toml`. If you change a default, add an entry to
  `SETTINGS_DEFAULT_MIGRATIONS` in `launcher/launcher.py` so existing configurations pick it up.
- Never include game files, ISOs, extracted assets, or anything else derived from the game's data.

By submitting a pull request, you agree that your contribution is licensed under the license of
the part of the project it changes (see [License](#license)).

## Reporting bugs

Before opening an issue, check the [known issues](#known-issues) and the existing issues. When you
open one, include:

- Your platform, operating system, CPU, GPU and graphics driver version.
- The release version, shown in the launcher's **About** tab.
- What you were doing and what happened. For graphical problems, add a screenshot.
- The log. The launcher's **Play** tab shows the game's console output, and its diagnostics
  section can create a support bundle with the logs and settings.

## Project layout

```
launcher/            Launcher and installer (Python, with a Qt WebEngine interface)
simpsons/            The game project: recompiled code (generated/), hand-written hooks (src/)
simpsons/re/         Notes on the game's internals
tools/rexglue-sdk/   ReXGlue runtime: kernel, audio, input and graphics layers
tools/XenonRecomp/   PowerPC to C++ static recompiler
tools/extract-xiso/  Xbox ISO extraction, used by the launcher's installer
tools/bench/         Unattended test and benchmark tooling
.github/workflows/   Build check on every push and pull request; manual release packaging
```

Not in the repository: game data, prebuilt toolchains (`tools/clang20`, `tools/rexglue-bin`) and
build output.

## Roadmap

The goal is for this to be the best way to play the game. In rough order:

- **Fully native renderer.** The Xbox 360 GPU emulation is being replaced piece by piece with
  native rendering, each step checked to give exactly the same image. Done: every shader compiled
  ahead of time, native replacements for the most expensive shaders, render-to-texture and copies
  done natively instead of through the emulated EDRAM, and render targets sized like native ones.
  Next: real vertex and index buffers, and textures and buffers uploaded when the game loads them
  instead of watching its memory. A native renderer is also what the features below build on.
- **No slowdowns or stutters.** A steady 60 FPS everywhere, including at 2x internal resolution
  on the Steam Deck, with no shader compilation hitches.
- **Widescreen.** Wider aspect ratios such as 21:9 without stretching.
- **Controller prompts.** Button icons that match the controller you have connected (Xbox,
  PlayStation, Nintendo, Steam Deck), or your keyboard keys, in every in-game prompt, menu and
  tutorial.
- **Game fixes.** Fixes for bugs in the original game.
- **Restored content.** Unused content from the original game, brought back where it works.
- **Mod support.** A mod loader and tools for replacing and adding scripts, models, objects and
  levels.
- **Android** port.

## Legal

*The Simpsons Game* is © 2007 Electronic Arts Inc. *The Simpsons* and all related characters and
trademarks belong to their respective owners. Xbox and Xbox 360 are trademarks of Microsoft
Corporation.

This is an unofficial, non-commercial fan project. It is not affiliated with, endorsed by or
sponsored by Electronic Arts, Fox, Microsoft or any other rights holder.

This repository and its releases contain no game assets: no textures, models, audio, video,
scripts or level data. The code in `simpsons/generated` is produced by recompiling the game's
executable code and does not contain the game's data. The game runs only with data installed from
an ISO of your own copy, and nothing from your installation is uploaded anywhere.

Please support the original release. Do not use this project with copies of the game you do not
own, and do not ask for or share game files in this repository's issues or discussions. You are
responsible for complying with the laws that apply to you regarding backups of media you own.

This software is provided as is, without warranty of any kind.

**A note on AI assistance:** parts of this project, including research into the game's internals,
build tooling and bug fixes, have been developed with the assistance of Claude AI, in the interest
of getting a working release out and turning around fixes as quickly as possible for a solo,
fan-made effort. Treat it as a fast-moving hobby project rather than a polished commercial
release.

## License

The project's own code (the launcher, the hand-written game code and the tooling) is licensed
under the [GNU General Public License v3.0](LICENSE). The ReXGlue SDK in `tools/rexglue-sdk` is
under the BSD 3-Clause License, as is the Xenia code it derives from. XenonRecomp in
`tools/XenonRecomp` is under the MIT License. Third-party libraries keep their own licenses; see
`tools/rexglue-sdk/thirdparty`.

## Credits

- [Xenia](https://xenia.jp), the Xbox 360 emulator research project whose code the runtime is
  built on.
- The ReXGlue SDK by Tom Clay, the recompilation runtime this port uses.
- [XenonRecomp](https://github.com/hedge-dev/XenonRecomp) by hedge-dev and contributors.
- tronuo, for the 60 FPS patch and the Havok physics fix.
- Everyone who has contributed code: Gabry179, awemancba, tronuo0 and anasalialamgir.
- The libraries the runtime is built with: SDL3, Dear ImGui, glslang, SPIRV-Tools, FFmpeg, spdlog,
  fmt and Tracy.
