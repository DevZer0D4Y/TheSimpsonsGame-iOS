# Runtime regressions

Build the matching SDK for macOS in Release, then run:

```sh
bash ios/tests/run.sh ios/rexglue-sdk /path/to/rexglue-sdk/out/install/mac-arm64 /path/to/rexglue-sdk/out/mac-arm64/Release
```

This uses the actual runtime to read a compact XISO and a full-disc ISO, checks case-insensitive paths, rejects nine malformed images, and verifies the guest clock freezes and resumes without counting suspended time. Fixtures are generated in a temporary directory and removed after the run. These tests do not measure iPhone gameplay performance or exercise UIKit suspension.

The runner also checks the lifecycle gate under continuous work, repeated pauses, early resume, delayed GPU preparation and shutdown while paused. The gate test can be compiled with `-fsanitize=thread` to check synchronization.

Run the command from the repository root. The upload-order regression checks 12,000 interleaved upload/draw versions under AddressSanitizer and UndefinedBehaviorSanitizer. The persistent-cache test uses the runtime filesystem API to check 64 reopen/append cycles, header and record retention, torn-tail recovery and failed opens on Darwin.

The touch-layout test rejects deferred B taps and the old B-to-right-stick long-press binding. It verifies 1,000 held-B polls through the guest input driver, simultaneous movement/A, release and inactive input. UIKit finger capture, off-button drift and cancellation still need device playtesting.
