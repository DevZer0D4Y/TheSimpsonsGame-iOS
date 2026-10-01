You need your own copy of The Simpsons Game for Xbox 360. No game content is included; the
launcher installs the game from your own ISO.

### Improvements

- Fixed a stall in every frame: presenting a frame made the game wait until the GPU had finished the previous one, and rebuilt a pipeline each time. Frame pacing is smoother at every resolution; at 2x internal resolution in Springfield on the Steam Deck, this fix alone raised the frame rate from 46 to 51 FPS.
- Fixed the lag in Springfield. Vertex data the game rewrites every frame is now uploaded fresh for each draw instead of being write-protected and faulted on hundreds of times per frame. On the Steam Deck, Springfield went from 51-54 to 59 FPS at 1x, and now holds 60.
- 2x internal resolution is now practical on the Steam Deck. Render-to-texture works natively at 2x instead of copying through emulated memory and reloading every texture, and resolved images are written to emulated memory only when something actually reads them. Springfield, the busiest area, went from 24 to about 52 FPS at 2x.
- Native replacement shaders for the most expensive post-processing passes are now used in play (before, they only loaded in testing), with versions for 1x and 2x internal resolution. They produce the same image as the translated shaders. The Linux packages include them.
- Faster texture sampling in shaders. Shaders whose textures are all unsigned skip the per-component signedness handling, and textures without mipmaps are sampled directly instead of through gradients. Both give identical images.
- Lower CPU use while the game waits for the GPU: its main thread sleeps instead of spinning, which leaves more of the Steam Deck's shared power budget for the GPU.
- Removed the "Instant character pop-in" patch and its warning. Characters are drawn correctly without it, and the launcher turns it off for anyone who still had it on.
- New launcher settings for the upscaling filter (bilinear, AMD FSR 1 or AMD CAS) and FSR sharpness.

### Known issues

- At 2x internal resolution, the busiest areas such as Springfield run at about 50 FPS on the Steam Deck. 1x holds 60.
- Frame rate settings above 60 do not add frames, because the game's frame scheduler tops out at 60 FPS, and they make frame pacing less even. 60 is recommended.
- At 60 FPS some scripted sequences can misbehave. If random deaths happen at the dam in "Lisa the Tree Hugger", switch to 30 for that section.

### Installing

- Windows: extract the zip and run `simpsons-launcher.exe`.
- Linux and Steam Deck: extract the archive and run `Play.sh`.
- CPU without AVX2 (most CPUs from before 2013): use the package ending in `-NoAVX2` instead.
- In the launcher's Install tab, select your Xbox 360 ISO, then press Play.
- Existing installs can update from the launcher's About tab.
