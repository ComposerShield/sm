#include "flashshift.h"
#include "variables.h"
#include "ida_types.h"
#include "funcs.h"
#include "sm_rtl.h"

// Flash Shift: Metroid Dread-style instant dash with 3-charge system
//
// Runs BEFORE RtlRunFrame so both the ROM and C code snapshots capture
// the modified position, camera, and VRAM tiles together. This prevents
// the ROM's RestoreSnapshot from reverting our changes.
//
// Movement is spread over several frames (fast dash) rather than an
// instant teleport, so the player can see Samus slide across the screen.

enum {
  kFlashShift_StepSize = 4,
  kFlashShift_StepsPerFrame = 5,    // 20px per frame (5 steps x 4px)
  kFlashShift_ShiftFrames = 4,      // frames of fast movement (4 x 20px = 80px = 5 tiles)
  kFlashShift_MaxCharges = 3,
  kFlashShift_Cooldown = 6,         // frames between uses
  kFlashShift_EmptyCooldown = 60,   // frames when all charges spent
  kFlashShift_RegenTime = 20,       // frames to regen one charge
  kFlashShift_Invincibility = 30,   // frames of post-shift i-frames (blink)
  kFlashShift_FlashFrames = 24,     // frames of white palette flash
  kFlashShift_FreezeFrames = 12,    // frames of forced zero momentum after shift
  kFlashShift_SfxDuration = 15,     // frames before cutting the SFX (~0.25s)
};

static bool fs_enabled;
static uint8 fs_charges = kFlashShift_MaxCharges;
static uint16 fs_regen_timer;
static uint16 fs_cooldown;
static uint16 fs_freeze_timer;      // frames remaining of post-shift movement freeze
static uint16 fs_flash_timer;       // frames remaining of white palette flash
static uint16 fs_sfx_timer;         // frames until we cut the shift SFX
static uint16 fs_shift_frames_left; // frames of dash movement remaining
static int32 fs_shift_step;         // per-step movement amount (sign = direction)
static bool fs_prev_combo;          // previous state of (G held + direction held)

bool FlashShift_IsEnabled(void) {
  return fs_enabled;
}

void FlashShift_SetEnabled(bool enabled) {
  fs_enabled = enabled;
  if (enabled && fs_charges == 0)
    fs_charges = kFlashShift_MaxCharges;
}

uint8 FlashShift_GetCharges(void) {
  return fs_charges;
}

static bool IsMovementTypeAllowed(void) {
  switch (samus_movement_type) {
  case kMovementType_00_Standing:
  case kMovementType_01_Running:
  case kMovementType_02_NormalJumping:
  case kMovementType_03_SpinJumping:
  case kMovementType_05_Crouching:
  case kMovementType_06_Falling:
  case kMovementType_0E_TurningAroundOnGround:
  case kMovementType_0F_CrouchingEtcTransition:
  case kMovementType_10_Moonwalking:
  case kMovementType_14_WallJumping:
  case kMovementType_15_RanIntoWall:
  case kMovementType_17_TurningAroundJumping:
  case kMovementType_18_TurningAroundFalling:
    return true;
  default:
    return false;
  }
}

// Snap camera to Samus and fully redraw the tilemap in VRAM.
static void SnapCameraAndRedraw(void) {
  int new_x = (int)(int16)samus_x_pos - 0x80;
  int max_x = (int)room_width_in_scrolls * 256 - 256;
  if (new_x < 0) new_x = 0;
  if (new_x > max_x) new_x = max_x;
  layer1_x_pos = (uint16)new_x;
  layer1_x_subpos = 0;

  CalculateLayer2Xpos();
  CalculateLayer2Ypos();
  CalculateBgScrolls();
  DisplayViewablePartOfRoom();
  // DisplayViewablePartOfRoom mutates block vars as a side effect
  // (increments them 17 times). Recalculate from scroll registers
  // so they're correct, then save as "previous" so the next frame's
  // incremental scroll updater sees no delta.
  CalculateBgScrollAndLayerPositionBlocks();
  UpdatePreviousLayerBlocks();
}

void FlashShift_Update(bool g_key_held) {
  if (!fs_enabled)
    return;

  // --- Timers that tick regardless of shift state ---

  // White palette flash (cycles during both dash and freeze)
  if (fs_flash_timer > 0) {
    fs_flash_timer--;
    samus_hurt_flash_counter = 3;
  }

  // SFX cutoff: queue a short replacement sound on channel 3.
  // Writing 0 to APUI03 doesn't stop the SPC — it just means "no new
  // request" and the sustained charge sound keeps playing. To actually
  // cut it, we must send a NEW sound that replaces it. Sound 0x1 (the
  // "stop health alarm" sound) is very short and effectively silences
  // the channel.
  if (fs_sfx_timer > 0) {
    if (--fs_sfx_timer == 0) {
      sfx_cur[2] = 0;
      sfx_state[2] = 0;
      sfx_readpos[2] = sfx_writepos[2];
      QueueSfx3_Max6(1);
    }
  }

  // --- Active dash movement (multi-frame) ---

  if (fs_shift_frames_left > 0) {
    fs_shift_frames_left--;

    // Move steps for this frame, stopping on wall collision
    for (int i = 0; i < kFlashShift_StepsPerFrame; i++) {
      Samus_MoveRight_NoSolidColl(fs_shift_step);
      if (samus_collision_flag) {
        fs_shift_frames_left = 0;
        break;
      }
    }

    // Zero momentum and hold animation during dash
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    samus_x_extra_run_speed = 0;
    samus_x_extra_run_subspeed = 0;
    samus_y_speed = 0;
    samus_y_subspeed = 0;
    samus_anim_frame_timer = 2;
    samus_collision_flag = 0;
    time_is_frozen_flag = 1;

    // Lock channel 3 state machine while our SFX is playing, so game
    // sounds queued DURING RtlRunFrame can't replace it.
    if (fs_sfx_timer > 0) {
      sfx_readpos[2] = sfx_writepos[2];
      sfx_state[2] = 2;
      sfx_clear_delay[2] = 99;
    }

    // Suppress scroll handler's delta computation
    samus_prev_x_pos = samus_x_pos;
    samus_prev_x_subpos = samus_x_subpos;

    // Update camera and VRAM each frame so the world scrolls smoothly
    SnapCameraAndRedraw();

    // When dash ends, transition to freeze phase
    if (fs_shift_frames_left == 0) {
      fs_cooldown = (fs_charges == 0) ? kFlashShift_EmptyCooldown : kFlashShift_Cooldown;
      fs_freeze_timer = kFlashShift_FreezeFrames;
    }
    return;
  }

  // --- Post-shift freeze ---

  if (fs_freeze_timer > 0) {
    fs_freeze_timer--;
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    samus_x_extra_run_speed = 0;
    samus_x_extra_run_subspeed = 0;
    samus_y_speed = 0;
    samus_y_subspeed = 0;
    samus_anim_frame_timer = 2;
    samus_collision_flag = 0;
    time_is_frozen_flag = 1;

    // Lock channel 3 state machine while our SFX is playing
    if (fs_sfx_timer > 0) {
      sfx_readpos[2] = sfx_writepos[2];
      sfx_state[2] = 2;
      sfx_clear_delay[2] = 99;
    }

    // Unfreeze movement handler when freeze ends
    if (fs_freeze_timer == 0)
      time_is_frozen_flag = 0;
  }

  // --- Charge regeneration ---

  if (fs_charges < kFlashShift_MaxCharges) {
    if (fs_regen_timer > 0) {
      fs_regen_timer--;
    } else {
      fs_charges++;
      if (fs_charges < kFlashShift_MaxCharges)
        fs_regen_timer = kFlashShift_RegenTime;
    }
  }

  // --- Cooldown ---

  if (fs_cooldown > 0) {
    fs_cooldown--;
    bool left = (joypad1_lastkeys & button_config_left) != 0;
    bool right = (joypad1_lastkeys & button_config_right) != 0;
    bool has_dir = (left || right) && !(left && right);
    fs_prev_combo = g_key_held && has_dir;
    return;
  }

  // --- Trigger detection ---

  bool left = (joypad1_lastkeys & button_config_left) != 0;
  bool right = (joypad1_lastkeys & button_config_right) != 0;
  bool has_dir = (left || right) && !(left && right);

  bool combo = g_key_held && has_dir;
  bool trigger = combo && !fs_prev_combo;
  fs_prev_combo = combo;
  if (!trigger)
    return;

  if (game_state != kGameState_8_MainGameplay)
    return;
  if (fs_charges == 0)
    return;
  if (!IsMovementTypeAllowed())
    return;

  // --- Execute shift ---

  // Spend charge, start regen
  fs_charges--;
  fs_regen_timer = kFlashShift_RegenTime;

  // Zero all momentum
  samus_x_base_speed = 0;
  samus_x_base_subspeed = 0;
  samus_x_extra_run_speed = 0;
  samus_x_extra_run_subspeed = 0;
  samus_y_speed = 0;
  samus_y_subspeed = 0;

  // Set collision direction and per-step amount
  if (right) {
    samus_collision_direction = 1;
    fs_shift_step = kFlashShift_StepSize << 16;
  } else {
    samus_collision_direction = 0;
    fs_shift_step = -(kFlashShift_StepSize << 16);
  }

  // Instantly face the shift direction and set a clean pose.
  // Breaks spin jumps into falling, prevents slow turn-around animation.
  {
    samus_pose_x_dir = right ? 0 : 4;
    bool airborne = (samus_movement_type == kMovementType_02_NormalJumping ||
                     samus_movement_type == kMovementType_03_SpinJumping ||
                     samus_movement_type == kMovementType_06_Falling ||
                     samus_movement_type == kMovementType_14_WallJumping ||
                     samus_movement_type == kMovementType_17_TurningAroundJumping ||
                     samus_movement_type == kMovementType_18_TurningAroundFalling);
    if (airborne) {
      // Stop the spin jump sound by queuing the spin-landing sound (0x32)
      // on channel 1. Writing 0 to APUI01 only means "no new request" and
      // doesn't stop the sustained spin sound — the game normally ends it
      // by replacing it with this short landing sound.
      if (samus_movement_type == kMovementType_03_SpinJumping ||
          samus_movement_type == kMovementType_14_WallJumping) {
        QueueSfx1_Max15(0x32);
      }
      samus_movement_type = kMovementType_06_Falling;
      samus_pose = right ? kPose_29_FaceR_Fall : kPose_2A_FaceL_Fall;
      samus_anim_frame = 0;
    } else {
      // Don't change movement type for ground shifts — forcing Standing
      // leaves the movement handler in an inconsistent internal state that
      // causes Samus to get stuck. Just update the pose for facing direction.
      samus_pose = right ? kPose_01_FaceR_Normal : kPose_02_FaceL_Normal;
    }
  }

  // Start multi-frame dash
  fs_shift_frames_left = kFlashShift_ShiftFrames;

  // Visual effects (run through both dash and freeze phases)
  samus_invincibility_timer = kFlashShift_Invincibility;
  samus_hurt_flash_counter = 3;
  fs_flash_timer = kFlashShift_FlashFrames;

  // Shinespark charge SFX on channel 3, cut short after ~0.25s
  RtlApuWrite(APUI03, 0);
  sfx_cur[2] = 0;
  sfx_state[2] = 0;
  sfx_readpos[2] = sfx_writepos[2];
  QueueSfx3_Max9(0xC);
  fs_sfx_timer = kFlashShift_SfxDuration;
}
