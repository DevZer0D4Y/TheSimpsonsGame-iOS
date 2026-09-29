You need your own copy of The Simpsons Game for Xbox 360. No game content is included; the
launcher installs the game from your own ISO.

### Improvements

- Characters and objects no longer go missing at the start of levels (#18, #30). Character meshes with absent optional vertex streams are now drawn safely on every GPU, including the Steam Deck.
- Fixed the GPU crash right after the first cutscene or on the first level load on AMD graphics cards with Windows, with both Vulkan and Direct3D 12 (#13, #16, #25). Those same character draws read GPU memory that had nothing behind it.
- New packages for older CPUs without AVX2, ending in `-NoAVX2` (#28). The standard package fails to start on those CPUs with error 0xc0000142. The launcher detects which one your CPU needs and its updater installs the right package.
- Fixed washed-out character colors on the native renderer. 10-bit render targets are now stored at full precision.
- Fixed audio crackle. The game is now asked for audio at the steady rate the Xbox 360 hardware used, instead of in bursts that made it hand over silent frames.
- Menus, the title screen and loading screens now run at the original 30 FPS, so menu navigation is no longer twice as fast and the title screen no longer stutters. Gameplay still runs at 60 FPS.
- Much lower CPU use. Threads waiting on several game events at once now sleep instead of polling; the audio thread dropped from about half a CPU core to under 5 percent, which also leaves more of the Steam Deck's power budget for the GPU.
- FXAA now works with the Vulkan renderer on Linux and Steam Deck, and new installs start with it on. It smooths the jagged cel-shading outlines at very little cost.
- New "Always show subtitles" setting in the launcher, which shows subtitles from the very first cutscene of a new game (#27; setting added by anasalialamgir in #31).
- The launcher can start the game without its window: `Play.sh --play` or `simpsons-launcher.exe --play`, for example from your own Steam shortcut (#18, #20).
- The Windows launcher is now built with a freshly compiled PyInstaller bootloader and carries version information, to reduce false antivirus detections such as Trojan:Win32/Suschil!rfn (#21, #32).
- The native renderer, which draws with the GPU's own render targets, is now the default. The Renderer setting can switch back to the accurate EDRAM emulation path.
- Faster rendering. Render target resolves happen in a single pass and write straight into the textures that sample them, and far fewer EDRAM emulation copies are made around clears and multisampled surfaces. In a captured gameplay frame, GPU time on the Steam Deck dropped from 14.6 ms to 8.1 ms.
- Lower GPU command processing overhead per register write and per shader constant update.
- The driver pipeline cache is saved between runs, reducing shader compilation stutter after the first session.
- With VSync off, guest vblank interrupts now follow the configured refresh rate instead of running at 1000 Hz, which cut GPU load in menus.
- Fixed intermittent stalls where a game thread was created but never started.
- Quitting the game can no longer hang. If shutdown gets stuck, the game exits after 10 seconds.
- New README with installation, configuration, build and contribution guides.

### Known issues

- Frame rate settings above 60 do not add frames, because the game's frame scheduler tops out at 60 FPS, and they make frame pacing less even. 60 is recommended.
- At 60 FPS some scripted sequences can misbehave. If random deaths happen at the dam in "Lisa the Tree Hugger", switch to 30 for that section.
- The "Instant character pop-in" patch remains experimental and is no longer needed.

### Installing

- Windows: extract the zip and run `simpsons-launcher.exe`.
- Linux and Steam Deck: extract the archive and run `Play.sh`.
- CPU without AVX2 (most CPUs from before 2013): use the package ending in `-NoAVX2` instead.
- In the launcher's Install tab, select your Xbox 360 ISO, then press Play.
- Existing installs can update from the launcher's About tab.
