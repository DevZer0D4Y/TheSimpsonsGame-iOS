# Changelog

## 0.2.4 — 2026-10-07

- Report touch B immediately and keep it held until the finger lifts or the touch is cancelled, including when the finger drifts outside the button.
- Preserve the minimum press pulse for quick B taps; long holds release immediately and cancellation clears the pulse.
- Remove the inherited FPS-layout long-press binding that deferred B and sent a right-stick click instead.
- Add a regression check for B's immediate, continuous mapping and the other default controls.

## 0.2.3 — 2026-10-07

- Preserve the shader and pipeline caches across launches on Darwin. Previously, append/read streams started at EOF and valid cache headers were missed.
- Disable audio sample scans, callback timing/logging, render-target statistics and readback pixel analysis unless diagnostics are explicitly enabled.
- Refresh embedded runtime libraries on every iOS build, including builds where only the SDK changed.
- Support clean source checkouts and archives with vendored dependency sources and the included generated arm64 code.

New shader encounters can still stall the first run. Repeat the same section after restarting to check cache reuse. Frame-rate improvement has not been quantified on the iPhone.

## 0.2.2 — 2026-10-06

- Preserve CPU upload snapshots after earlier GPU draws have read them, addressing the flickering EA/legal text under the title logo.
- Make expensive GPU timing, texture census and reload logging opt-in.

## 0.2.1 — 2026-10-06

- Correct the freeze when returning from another app using cancellable worker pauses, packet-level GPU checkpoints and bounded lifecycle acknowledgement waits.

## 0.2.0 — 2026-10-06

- Pause rendering, audio and guest time when inactive and resume the existing session on return.
- Add direct Xbox 360 ISO/XISO loading and safe image import.
- Opt into Apple Game Mode on supported iOS versions.
- Prevent late vblank catch-up bursts and disable intrusive GPU diagnostics by default.
