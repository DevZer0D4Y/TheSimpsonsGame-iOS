# The Simpsons Game for iOS

A native iPhone and iPad port of **The Simpsons Game (Xbox 360, 2007)**, built by statically recompiling the original PowerPC code to arm64 with [ReXGlue](https://github.com/rexglue/rexglue-sdk). Graphics run through Vulkan on Metal using MoltenVK.

**You need your own copy of the Xbox 360 game. Game assets and disc images are not included.**

**[Releases](https://github.com/DevZer0D4Y/TheSimpsonsGame-iOS/releases)** · [Changelog](CHANGELOG.md) · [Detailed setup guide](ios/simpsons/README.md)

If you want to support me and remain up-to-date about this and other projects, you can in different ways:

Ko-Fi: https://ko-fi.com/dev_zer0

Discord: https://discord.gg/uFChheZEWX

YouTube: https://www.youtube.com/@develop_erZ

## Features

- Direct Xbox 360 ISO/XISO loading, without extracting the disc.
- Support for extracted game files.
- On-screen touch controls and controller support.
- Optional 60 FPS gameplay with the community Havok physics fix.
- Pause and resume when switching apps or locking the phone.
- Apple Game Mode support on compatible devices and iOS versions.
- Persistent shader and pipeline caches across launches.
- Automatic language selection from the languages available on your disc.

## Requirements

- An iPhone or iPad running **iOS/iPadOS 16 or later**. Development and playtesting have used an **iPhone 14 Pro Max**.
- Your own Xbox 360 copy of *The Simpsons Game*, title ID **45410809**, as an ISO/XISO or extracted disc files.
- Enough free storage for the game data. Importing an ISO copies it into the app's storage.
- A sideloading tool to sign and install the IPA, such as ESign, AltStore, SideStore or Sideloadly.

A controller is recommended, but the game can also be played with touch controls.

## Installation

1. Obtain the IPA from a release or [build it from source](#building).
2. Sign and install it with your preferred sideloading tool. With ESign, import the IPA, then sign and install it inside ESign.
3. Open **The Simpsons Game** and tap **Choose ISO**.
4. Select your Xbox 360 ISO/XISO in the Files app. Wait for the import to finish, then launch the game.

To update an existing installation, use the same signing identity and bundle identifier to keep the app's data.

### Game files

You can also copy an image named `game.iso` into **Files → On My iPhone → The Simpsons Game**. The app mounts it directly, and it takes priority over extracted files.

For an extracted disc, place the files in `game/`, with `default.xex` directly inside that folder:

```text
The Simpsons Game/
├── game.iso            Imported disc image, if using an ISO
├── game/               Extracted disc files, if using a folder
│   ├── default.xex
│   ├── movies/
│   └── audiostreams/
├── userdata/           Saves and profile data
├── cache/              Shader and pipeline caches
└── simpsons.toml       Settings
```

Keep the disc's original paths and file names. The app creates `userdata/`, `cache/` and `simpsons.toml`. On European discs, it prefers the phone's language when that language is available in the game data.

## Playing and settings

The touch gamepad appears automatically when no controller is connected. Edit `simpsons.toml` in the Files app to change settings, then restart the app for the changes to apply.

| Setting | Default | Description |
|---|---|---|
| `unlock_60fps` | `true` | Targets 60 FPS during gameplay. Set to `false` for the original 30 FPS. Menus and loading screens remain at 30 FPS. |
| `subtitles` | `false` | Enables subtitles, including the first cutscene of a new game. |
| `present_letterbox` | `false` | Fills the screen. Set to `true` to preserve the original 16:9 aspect ratio with side bars. |
| `touch_controls` | `"auto"` | Shows controls when no controller is connected. Also accepts `"on"` or `"off"`. |
| `user_language` | Automatic | Overrides language selection: `1` English, `3` German, `4` French, `5` Spanish, `6` Italian. |

Switching apps or locking the phone pauses rendering, audio and game time. Returning resumes the same session while the app remains in memory. iOS can terminate a suspended app under memory pressure; save progress before leaving a long session.

## Status and known issues

Current version: **0.2.4 (build 6)**.

B now stays pressed from touch-down until your finger lifts, including when it drifts outside the button. Holding B no longer sends a right-stick click.

This release includes the fixes for freezing after app switches, flickering EA/legal text beneath the title logo, and shader caches failing to reload on later launches. It also removes unnecessary diagnostic work during normal play.

- Newly encountered shaders and demanding scenes can still cause FPS drops. Previously cached shaders can be reused after restarting; frame-rate improvement has not been quantified on the iPhone.
- Some scripted sequences can misbehave at 60 FPS, including the dam section in **Lisa the Tree Hugger**. Use `unlock_60fps = false` for that section if needed.
- Apple Game Mode availability depends on the device and iOS version.

See the [changelog](CHANGELOG.md) for the changes in each version.

## Building

Requires an **Apple Silicon Mac**, **Xcode 16 or newer**, **CMake 3.25+**, **Ninja** and **Python 3**. The release build used Xcode 27.0.

From the repository root:

```sh
bash ios/build.sh
```

The script builds the matching runtime and app, then packages:

```text
ios/simpsons/out/TheSimpsonsGame.ipa
```

Generated C++ sources and the matching runtime dependencies are included. Ordinary builds do not require a game disc or a host code-generation tool. The first build downloads pinned MoltenVK build dependencies into the ignored SDK cache. The resulting IPA needs signing with your sideloading tool before installation.

Build parallelism defaults to two jobs. To change it:

```sh
JOBS=3 bash ios/build.sh
```

For manual builds, code regeneration and device transfer instructions, see the [detailed build guide](ios/simpsons/README.md#building).

## Source layout

| Path | Contents |
|---|---|
| [`ios/simpsons/`](ios/simpsons/) | App, platform integration, game hooks and generated C++ |
| [`ios/rexglue-sdk/`](ios/rexglue-sdk/) | Matching ReXGlue runtime and vendored dependencies |
| [`ios/tests/`](ios/tests/README.md) | ISO, lifecycle, timing, upload and shader-cache regression checks |
| [`ios/build.sh`](ios/build.sh) | Runtime build, app build and IPA packaging |

The original desktop sources remain in the repository root. See [ios/README.md](ios/README.md) for an overview of the iOS build.

## Credits

- [TheSimpsonsGameRecomp](https://github.com/YesterMester/TheSimpsonsGameRecomp) by **YesterMester** and contributors.
- **tronuo** for the 60 FPS patch and Havok physics fix.
- [ReXGlue](https://github.com/rexglue/rexglue-sdk) by **Tom Clay**, and the [Xenia](https://xenia.jp) project.
- MoltenVK, SDL3, FFmpeg and the other libraries included with the runtime.

## License

Project source is licensed under [GPL-3.0](LICENSE). Vendored libraries retain their own licenses and attribution.

This is an unofficial fan project, unaffiliated with Electronic Arts, the owners of *The Simpsons*, or Microsoft. Original game content and trademarks belong to their respective owners. No game assets, disc images, signing certificates or provisioning profiles are distributed with this source.

## Support and community

[Ko-Fi](https://ko-fi.com/dev_zer0) · [Discord](https://discord.gg/uFChheZEWX) · [YouTube](https://www.youtube.com/@develop_erZ)
