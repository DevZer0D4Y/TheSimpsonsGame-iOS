// 60 FPS mode.
//
// The game shipped at 30 FPS. Two patches by tronuo (the community 60 FPS
// patch for title 45410809 in xenia-canary/game-patches, and the Havok fix
// TheSimpsonsGameRecomp carries) make it run at 60. Upstream edits them into
// its generated code; here they are mid-function hooks (config/hooks.toml), so
// they survive regenerating the code and can be switched off.
//
//  1. sub_82867A48 passes the frame scheduler a swap interval of 2 vblanks
//     ("li r4,2" at 0x82867A70). 1 makes every frame one 60 Hz vblank.
//  2. sub_827A55C0 steps the Havok world with a fixed 1/30 s timestep (the
//     float at 0x82159238, loaded at 0x827A563C and 0x827A5660). At 60 FPS it
//     must be 1/60 s, or physics runs at double speed.
//
// Menus, the title screen and loading screens still run at 30 (frame_pacing.cpp).

#include <cstdint>
#include <cstring>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/ppc.h>

REXCVAR_DEFINE_BOOL(unlock_60fps, true, "Game",
                    "Run gameplay at 60 FPS (community 60 FPS patch plus the Havok physics "
                    "fix); false keeps the original 30 FPS. Applies on the next launch");

namespace {

bool Enabled() {
  // Read once: the swap interval is set at startup, and the physics step must
  // not change mid-session.
  static const bool enabled = REXCVAR_GET(unlock_60fps);
  return enabled;
}

double FloatBits(uint32_t bits) {
  float value;
  std::memcpy(&value, &bits, sizeof(value));
  return double(value);
}

// 1/60 as the guest's single-precision constant (0.0166667f).
const double kHavokStep60 = FloatBits(0x3C88AB86);

}  // namespace

// After "li r4,2": the swap interval sub_82718D48 receives.
void SimpsonsSwapIntervalHook(PPCRegister& r4) {
  if (Enabled()) {
    r4.u64 = 1;
  }
}

// After "lfs f0,-28104(r30)": the step a frame's time is compared against.
void SimpsonsHavokCompareStepHook(PPCRegister& f0) {
  if (Enabled()) {
    f0.f64 = kHavokStep60;
  }
}

// After "lfs f1,-28104(r30)": the step passed to the Havok world.
void SimpsonsHavokStepHook(PPCRegister& f1) {
  if (Enabled()) {
    static bool logged = false;
    if (!logged) {
      logged = true;
      REXLOG_INFO("60 FPS mode: Havok steps {} s instead of {} s", kHavokStep60, f1.f64);
    }
    f1.f64 = kHavokStep60;
  }
}
