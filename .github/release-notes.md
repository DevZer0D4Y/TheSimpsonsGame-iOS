You need your own copy of The Simpsons Game for Xbox 360. No game content is included; the
launcher installs the game from your own ISO.

### Keyboard and mouse

- The game takes the mouse as soon as its window is active: no more clicking into the window, the cursor is hidden and stays inside, and switching to another window lets go of it. On Linux and the Steam Deck, mouse look reads the mouse's raw movement, so it is smooth at any speed and free of desktop pointer acceleration.
- Every control can be rebound in the launcher (Settings, Keyboard & mouse): press Change, then a key, a mouse button or the wheel. An action can have more than one key, and the mouse wheel and side buttons can be bound. Mouse sensitivity and invert-Y are there too.
- Press F1 in the game to see the controls: each controller button, the key it is on and what it does, so the game's button prompts make sense on a keyboard. A short reminder appears when the game starts.
- Controls can also be changed while playing: F4 opens the settings, with the controls under Input > Keybinds. The mouse is released while the settings or the console are open, so they can be clicked.
- New default layout: W A S D to move, the mouse to look around, Space to jump, left click to attack, right click for the special attack, E for actions, Shift to target, Ctrl to walk, 1 to 4 or the arrow keys to switch character, Tab for the to-do list and Esc to pause. Enter and Backspace confirm and go back in menus.
- The camera can be turned with keys too: bind Look up, down, left and right in the launcher (#33).
- The keys that open the in-game overlays (controls list F1, settings F4, FPS overlay F3, console `) can be changed in the launcher, and a key changed in the game's own settings now stays changed after a restart.
- Quick key taps and clicks are no longer missed.

### Launcher

- A new look with the show in mind, in a light theme (Springfield by day) and a dark theme (Springfield by night). The button at the top right switches between them; until you pick one, the launcher follows your system's setting.
- Settings are saved the moment you change them. Before, a change was lost unless you pressed Save at the bottom of the page, which is why settings seemed to reset, fullscreen didn't stick and a higher render resolution never took effect (#34).
- The sharpening setting only appears with FSR 1, the only filter that uses it (Bilinear never sharpens), and Render resolution says what it does: 3x draws a true 4K picture (#34).
- Launcher artwork works again. With the ffmpeg bundled on Windows, every cutscene frame failed, which left the art folder empty; elsewhere the same pictures repeated. Every install makes its artwork again once (#7).
- The launcher window no longer stops at "127.0.0.1 refused to connect" when its built-in server answers late: it retries, and if the server never answers it says what to try. It also writes `launcher.log` next to itself, for bug reports (#37).

### Windows

- The game now asks Windows for its finest timer resolution, as Xenia does. Without it, every short wait in the game, its frame pacing and the threads that feed the audio stretched to Windows' default 15.6 ms tick, the likely cause of choppy audio (#36) and uneven frame pacing on Windows.

### Renderer

- Springfield at 2x internal resolution now runs at about 59 FPS on the Steam Deck, up from about 50. The GPU needs about a third less time per frame there: memory the game rewrites every frame is uploaded only when it actually changed, buffers are no longer all uploaded again every frame, and textures the game renders to keep the GPU's color compression on AMD GPUs.
- Every shader the game uses comes precompiled in the Linux and Steam Deck packages, so new areas no longer stop to translate shaders the first time they appear.
- Render targets are the size of what the game draws instead of covering the Xbox 360's whole EDRAM, which saves about 95 MB of video memory at 2x.
- Rendered images that no texture reads yet are written straight to memory, and shadow-map copies whose data is never used are skipped.
- Every change gives the same image as 0.0.6.1 in replay tests at 1x and 2x.

### Known issues

- In-game prompts still show controller buttons. Press F1 to see which key each one is on.
- Keyboard and mouse has been tested much less on Windows than on Linux and the Steam Deck.
- At 2x, occasional frame drops remain in the busiest areas (a few per minute in Springfield). They come from the game's frame timing on the CPU, not from the GPU.
- Frame rate settings above 60 do not add frames, because the game's frame scheduler tops out at 60 FPS, and they make frame pacing less even. 60 is recommended.
- At 60 FPS some scripted sequences can misbehave. If random deaths happen at the dam in "Lisa the Tree Hugger", switch to 30 for that section.

### Installing

- Windows: extract the zip and run `simpsons-launcher.exe`.
- Linux and Steam Deck: extract the archive and run `Play.sh`.
- CPU without AVX2 (most CPUs from before 2013): use the package ending in `-NoAVX2` instead.
- In the launcher's Install tab, select your Xbox 360 ISO, then press Play.
- Existing installs can update from the launcher's About tab.
