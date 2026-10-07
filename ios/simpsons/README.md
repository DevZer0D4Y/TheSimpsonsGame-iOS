# The Simpsons Game for iPhone

An iOS port of [TheSimpsonsGameRecomp](https://github.com/YesterMester/TheSimpsonsGameRecomp), the
static recompilation of *The Simpsons Game* (Xbox 360, 2007). The game's PowerPC code is translated
ahead of time to C++ and compiled for arm64; the [ReXGlue](https://github.com/rexglue/rexglue-sdk)
runtime (derived from Xenia) supplies the Xbox 360 kernel, audio and input, and the graphics run
through Vulkan on Metal (MoltenVK).

**No game content is included. You need your own copy of the Xbox 360 game.**

## Requirements

- An iPhone or iPad with iOS 16 or later. Developed on an iPhone 14 Pro Max (6 GB).
- *The Simpsons Game* for Xbox 360, extracted from your disc (title ID 45410809). The USA disc and
  the European discs share the same executable; a European disc carries its movies and speech in
  one folder per language (`movies/it`, `movies/es`, ...).
- About 4 GB of free space for the game files (one language).
- A Bluetooth controller is recommended. Without one, an on-screen gamepad appears. Holding B keeps it pressed until the finger lifts, even if it drifts off the button; it does not trigger a right-stick click.

## Installing

1. Build `TheSimpsonsGame.ipa` (see below) and sideload it with ESign, AltStore, SideStore,
   Sideloadly or TrollStore. The IPA is unsigned; the sideloading tool signs it.
2. On first launch, use **Choose ISO** to import an Xbox 360 ISO/XISO directly. The image is mounted read-only without extraction. Alternatively, copy an image named `game.iso` into the app’s Documents folder in Files; it takes priority over extracted data. For extracted data, follow step 3. Saves stay in `userdata/`.
3. Copy the extracted game into the app's `game` folder, with the Files app (On My iPhone >
   The Simpsons Game) or from a computer (Finder's Files tab, or `ios/push-game-data.sh`):

   ```
   The Simpsons Game/
   ├── game/              the extracted disc: default.xex, movies/, audiostreams/, ...
   ├── userdata/          saves (created by the app)
   ├── cache/             shader cache (created by the app)
   └── simpsons.toml      settings (created by the app)
   ```

   `game/` must contain `default.xex` directly. On a European disc only one language is needed:
   keep `movies/<lang>` and `audiostreams/<lang>` for the language you want and delete the
   others. The `$systemupdate` folder is not needed.

The game picks the language from the folders present (including inside an ISO), preferring the phone's own language.

The app opts into Apple Game Mode on supported iOS versions. Switching apps or locking the phone pauses rendering, audio and guest time and resumes the existing session. iOS may reclaim suspended apps under memory pressure; saved progress persists. GPU development telemetry is now opt-in, and late vblanks no longer generate catch-up bursts.

## Settings

`simpsons.toml` can be edited in the Files app; changes apply on the next launch.

| Setting | Default | |
|---|---|---|
| `unlock_60fps` | `true` | 60 FPS gameplay (community 60 FPS patch plus the Havok physics fix). `false` is the original 30 FPS. Menus and loading screens always run at 30. |
| `subtitles` | `false` | Always show subtitles, including the first cutscene of a new game. |
| `present_letterbox` | `false` | `false` stretches the 16:9 image to the screen; `true` keeps the aspect with side bars. |
| `touch_controls` | `"auto"` | On-screen gamepad: `"auto"` while no controller is connected, `"on"`, `"off"`. |
| `user_language` | from the disc | Xbox language number (1 English, 3 German, 4 French, 5 Spanish, 6 Italian). |

At 60 FPS some scripted sequences can misbehave, as on PC (the dam in "Lisa the Tree Hugger");
switch to 30 for that section if it happens.

## Building

Needs an Apple Silicon Mac with Xcode 16+, CMake 3.25+, Ninja and Python 3. Xcode 27.0 was used for the release build.

From the repository root:

```sh
bash ios/build.sh
```

This builds and installs the matching SDK, compiles the included generated C++ and writes `ios/simpsons/out/TheSimpsonsGame.ipa`. First-run build dependencies are downloaded into the SDK's ignored `out/deps` cache. The IPA is prepared for ESign signing; the main executable is signed by the sideloading tool. To request a local ad hoc main signature, use `ADHOC_MAIN_SIGN=1 bash ios/build.sh`.

For manual builds:

```sh
cd ios/rexglue-sdk
cmake --preset ios-arm64 -DREXGLUE_REGISTER_PACKAGE=OFF
cmake --build --preset ios-arm64-release --parallel 2
cmake --install out/build/ios-arm64 --config Release
export REXGLUE_IOS_SDK="$PWD/out/install/ios-arm64"
cd ../simpsons
cmake --preset ios-arm64-release -DSIMPSONS_REGENERATE_CODE=OFF
cmake --build --preset ios-arm64-release --parallel 2
ADHOC_MAIN_SIGN=0 bash ios/make-ipa.sh
```

To regenerate code after changing the manifest or hooks, build the SDK's host `rexglue` tool using its `mac-arm64` preset, put your `default.xex` in `ios/game_xex/`, set `REXGLUE_HOST_TOOL` to that host executable and configure the app with `-DSIMPSONS_REGENERATE_CODE=ON`. Ordinary builds use the included generated sources and do not require this step.

To copy the built IPA to ESign, use Finder file sharing or run `bash ios/send-to-esign.sh` from `ios/simpsons/` with `DEVICE` set to your paired iPhone's identifier. Wi-Fi access requires pairing and a reachable, unlocked phone. ESign bundle identifiers can vary; override `ESIGN` when necessary. Sign/install within ESign.

The same project builds natively for macOS (`mac-arm64-release` preset, with a mac-arm64 SDK
install in `REXGLUE_MAC_SDK`), which runs the same recompiled code and GPU path and is how the
port is tested without a phone: `--game_data_root=<extracted game>`, plus
`--vulkan_texture_bc_decompress=true` to decode textures the way an iPhone must.

### What differs from the PC project

- Code is regenerated with ReXGlue 0.10.0 instead of shipping upstream's 0.8.0 output.
  `config/functions.toml` declares the functions the analyzer misses; the CRT's setjmp/longjmp
  (which Lua's error handling depends on) are given to the code generator; the 60 FPS and Havok
  patches are mid-function hooks (`config/hooks.toml`, `src/fps_unlock.cpp`) instead of edits
  to generated files.
- The game hooks from upstream (`frame_pacing.cpp`, `ring_wait.cpp`, `subtitles.cpp`) are used
  as they are.
- Runtime fixes ported from upstream's SDK: the VP6 video pack-instruction aliasing (green
  video), characters drawn with absent optional vertex streams (invisible characters, zero
  fetches for size-0 streams), 10:10:10:2 render targets (bleached colors), all user slots
  signed in (saving), stale page protection recovery, GPU ring-wait sleeping.
- Apple-specific: CPU writes invalidate only their own 16 KB page of GPU-visible memory
  (`shared_memory_invalidation_widen_kb`); the default 64-page widening covered 1 MB on 16 KB
  pages and re-uploaded hundreds of MB/s.

## Testing without a phone

The app reads two environment variables (`src/test_harness.h`):

- `SIMPSONS_AUTOPLAY`: a scripted gamepad, e.g. `50:START,60-100/3:A,100-200/0.2:LY+:0.2`.
- `SIMPSONS_SHOTS`: seconds between screenshots of the game's image, saved to `userdata/shots`.

## Legal

*The Simpsons Game* © 2007 Electronic Arts Inc. *The Simpsons* © 20th Television. Xbox 360 is a
trademark of Microsoft. This is an unofficial fan project, not affiliated with or endorsed by any of
them. It contains no game assets and runs only with data from a copy of the game you own.

## Credits

- [TheSimpsonsGameRecomp](https://github.com/YesterMester/TheSimpsonsGameRecomp) by YesterMester
  and contributors, including tronuo for the 60 FPS patch and the Havok fix.
- The [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) by Tom Clay, and [Xenia](https://xenia.jp).
- MoltenVK, SDL3, FFmpeg and the other libraries listed in the SDK.

Release fixes and validation limits are listed in [the changelog](../../CHANGELOG.md). Shader caches persist across launches; replay a section after restarting to check cache reuse. Newly encountered shaders and demanding scenes may still cause frame drops.
