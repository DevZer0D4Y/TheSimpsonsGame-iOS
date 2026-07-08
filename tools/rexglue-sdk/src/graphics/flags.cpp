/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2020 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 *
 * @modified    Tom Clay, 2026 - Adapted for ReXGlue runtime
 */

#include <rex/graphics/flags.h>
#include <rex/logging.h>
#include <rex/ui/renderdoc_api.h>

REXCVAR_DEFINE_BOOL(gpu_allow_invalid_fetch_constants, false, "GPU",
                    "Allow invalid fetch constants");
REXCVAR_DEFINE_BOOL(native_2x_msaa, true, "GPU", "Enable native 2x MSAA");
REXCVAR_DEFINE_BOOL(depth_float24_round, false, "GPU", "Round float24 depth values");
REXCVAR_DEFINE_BOOL(depth_float24_convert_in_pixel_shader, false, "GPU",
                    "Convert float24 depth in pixel shader");
REXCVAR_DEFINE_BOOL(depth_transfer_not_equal_test, true, "GPU",
                    "Use not-equal test for depth transfer");
REXCVAR_DEFINE_BOOL(gamma_render_target_as_unorm16, true, "GPU",
                    "Use R16G16B16A16_UNORM for gamma render targets (more accurate than sRGB)")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);
REXCVAR_DEFINE_STRING(dump_shaders, "", "GPU", "Path to dump shaders to");
REXCVAR_DEFINE_BOOL(use_fuzzy_alpha_epsilon, false, "GPU",
                    "Use approximate compare for alpha test values to prevent "
                    "flickering on NVIDIA graphics cards");
REXCVAR_DEFINE_BOOL(gpu_debug_markers, false, "GPU",
                    "Insert debug markers into GPU command streams for tools "
                    "like PIX and RenderDoc. Automatically enabled when "
                    "RenderDoc is detected.");
// HAND PATCH: defensive guard against GPU hangs (amdgpu ring gfx timeout ->
// VK_ERROR_DEVICE_LOST) from a translated shader's main control-flow dispatch
// loop looping far more than any legitimate Xenos LOOP (8-bit count, max 4
// nested levels -> at most ~1020 legitimate passes) or JMP/CALL chain ever
// would. Investigated for the 2026-07-05 level-load hang (RADV ring timeouts
// mid-DRAW_INDEX on a skinned-mesh vertex shader as ESGS); the specific
// translated LOOP constructs examined were proven statically bounded, so this
// is a mitigation rather than a confirmed root-cause fix -- it exists in case
// some other loop/jump shape (or corrupted loop-constant data from elsewhere)
// produces genuinely unbounded control flow. 0 disables the guard.
REXCVAR_DEFINE_INT32(gpu_shader_max_cf_iterations, 100000, "GPU",
                    "Hard cap on passes through a translated shader's main "
                    "control-flow dispatch loop per invocation, as a guard "
                    "against GPU hangs from runaway translated control flow. "
                    "0 disables the guard.")
    .range(0, INT32_MAX);

// HAND PATCH: replace non-finite vertex positions (NaN/Inf, e.g. from 0/0 in
// skinning weight normalization when optional streams are null) with zeros in
// translated vertex shaders. NaN primitives wedge the rasterizer on
// RADV/VanGogh (pixel pipe never drains -> ring timeout).
REXCVAR_DEFINE_BOOL(gpu_sanitize_vertex_position, true, "GPU",
                    "Neutralize NaN/Inf vertex shader position outputs (degenerate instead).");

// HAND PATCH: the NaN/Inf/w==0 checks above only catch positions that are
// already IEEE-poison. They do NOT catch a position that is merely huge but
// finite -- which is exactly what a skinning shader produces if it indexes an
// UNINITIALIZED or STALE float-constant register as a bone matrix (this
// game's shaders select bone rows via a dynamically-computed address register
// `a0`, e.g. `c[52+a0]`; see spirv_translator.cpp SanitizeVertexPosition for
// the full writeup). A garbage bit pattern read as a float is far more often
// a huge finite number than exactly NaN/Inf, so it sails through the existing
// sanitizer, reaches the fixed-function clip/NDC transform, and can still
// wedge the RADV/VanGogh scan converter on a degenerate huge-magnitude
// triangle. No legitimate camera-space vertex in this game's content ever
// needs a clip-space component anywhere near this bound. 0 disables the
// check.
REXCVAR_DEFINE_DOUBLE(gpu_vertex_position_magnitude_limit, 1.0e6, "GPU",
                    "Clip-space vertex position component magnitude past which the position is "
                    "treated as poison and replaced (0 disables).")
    .range(0.0, 1.0e30);

bool IsGpuDebugMarkersEnabled() {
  static bool cached = false;
  static bool result = false;
  if (!cached) {
    cached = true;
    if (REXCVAR_GET(gpu_debug_markers)) {
      result = true;
      REXLOG_INFO("GPU debug markers enabled via CVar");
    } else {
      auto renderdoc_api = rex::ui::RenderDocAPI::CreateIfConnected();
      if (renderdoc_api) {
        result = true;
        REXLOG_INFO("GPU debug markers auto-enabled (RenderDoc detected)");
      }
    }
  }
  return result;
}
