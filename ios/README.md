# iOS source

Version 0.2.4 (build 6).

- `simpsons/`: app, platform integration, game hooks and generated arm64 C++.
- `rexglue-sdk/`: matching ReXGlue 0.10.0 runtime and vendored dependency sources.
- `tests/`: ISO, clock, lifecycle, upload-order and persistent-cache regressions.
- `build.sh`: build and package an IPA for a sideloading signer.

On an Apple Silicon Mac with Xcode 16 or newer, CMake 3.25+, Ninja and Python 3:

```sh
bash ios/build.sh
```

The release was built with Xcode 27.0. The script downloads MoltenVK's pinned build dependencies on the first run, installs the SDK into `ios/rexglue-sdk/out/install/ios-arm64`, and writes `ios/simpsons/out/TheSimpsonsGame.ipa`. Set `JOBS=3` to change build parallelism. No host code-generation tool or game disc is needed for the ordinary build; generated sources are included.

Dependencies are vendored source directories, so this iOS build does not require `git submodule update`. Their licenses remain in each dependency directory. Build outputs, private game data and signing files are ignored by Git.

Sign the IPA using ESign or another sideloading tool before installing it. Installation, disc setup, optional code regeneration and settings are described in [simpsons/README.md](simpsons/README.md). The source carries no personal signing identity.

The app pauses while inactive and resumes on return. iOS may reclaim a suspended process under memory pressure. Saves persist in `userdata/`. Game Mode activation depends on the device and iOS version.

The shader-cache fix allows previously encountered pipelines to be preloaded on later launches. Newly encountered shaders and demanding scenes may still drop frames; on-device frame-rate improvement has not been quantified.

See [the changelog](../CHANGELOG.md) for the fixes included in this release.
