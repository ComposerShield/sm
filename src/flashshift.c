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

enum {
  kFlashShift_StepSize = 4,
  kFlashShift_Steps = 20,           // 80px total / 4px per step (5 tiles)
  kFlashShift_MaxCharges = 3,
  kFlashShift_Cooldown = 6,         // frames between uses
  kFlashShift_EmptyCooldown = 60,   // frames when all charges spent
  kFlashShift_RegenTime = 20,       // frames to regen one charge
  kFlashShift_Invincibility = 30,   // frames of post-shift i-frames (blink)
  kFlashShift_FlashFrames = 24,     // frames of white palette flash
  kFlashShift_FreezeFrames = 10,    // frames of forced zero momentum
  kFlashShift_SfxDuration = 15,    // frames before cutting the SFX (~0.25s)
};

static bool fs_enabled;
static uint8 fs_charges = kFlashShift_MaxCharges;
static uint16 fs_regen_timer;
static uint16 fs_cooldown;
static uint16 fs_freeze_timer;  // frames remaining of post-shift movement freeze
static uint16 fs_flash_timer;   // frames remaining of white palette flash
static uint16 fs_sfx_timer;     // frames until we cut the shift SFX
static bool fs_prev_combo;      // previous state of (G held + direction held)

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

void FlashShift_Update(bool g_key_held) {
  if (!fs_enabled)
    return;

  // Post-shift movement freeze: zero all momentum and hold animation
  if (fs_freeze_timer > 0) {
    fs_freeze_timer--;
    samus_x_base_speed = 0;
    samus_x_base_subspeed = 0;
    samus_x_extra_run_speed = 0;
    samus_x_extra_run_subspeed = 0;
    samus_y_speed = 0;
    samus_y_subspeed = 0;
    // Hold animation timer at 2 so Samus_Animate() decrements it to 1
    // but never reaches 0 (which would advance the frame).
    samus_anim_frame_timer = 2;
  }

  // Post-shift flash: keep resetting hurt_flash_counter to 3 so the
  // white palette flash cycles continuously (odd=white, even=normal).
  // Value 3 skips the damage SFX trigger at counter==2.
  if (fs_flash_timer > 0) {
    fs_flash_timer--;
    samus_hurt_flash_counter = 3;
  }

  // Cut the shift SFX after the timer expires.
  // Fully reset channel 3: silence the APU, clear the current sound,
  // reset the state machine, and flush the queue so pending footstep
  // sounds don't immediately replace our sound mid-play.
  if (fs_sfx_timer > 0) {
    if (--fs_sfx_timer == 0) {
      RtlApuWrite(APUI03, 0);
      sfx_cur[2] = 0;
      sfx_state[2] = 0;
      sfx_readpos[2] = sfx_writepos[2];
    }
  }

  // Tick charge regeneration
  if (fs_charges < kFlashShift_MaxCharges) {
    if (fs_regen_timer > 0) {
      fs_regen_timer--;
    } else {
      fs_charges++;
      if (fs_charges < kFlashShift_MaxCharges)
        fs_regen_timer = kFlashShift_RegenTime;
    }
  }

  // Tick cooldown
  if (fs_cooldown > 0) {
    fs_cooldown--;
    // Still track combo state during cooldown so we don't fire on release
    bool left = (joypad1_lastkeys & button_config_left) != 0;
    bool right = (joypad1_lastkeys & button_config_right) != 0;
    bool has_dir = (left || right) && !(left && right);
    fs_prev_combo = g_key_held && has_dir;
    return;
  }

  // Determine direction from joypad (uses previous frame's keys, which is fine)
  bool left = (joypad1_lastkeys & button_config_left) != 0;
  bool right = (joypad1_lastkeys & button_config_right) != 0;
  bool has_dir = (left || right) && !(left && right);

  // Edge-detect the combination of G + direction.
  // Triggers whether you press G then direction, or direction then G.
  bool combo = g_key_held && has_dir;
  bool trigger = combo && !fs_prev_combo;
  fs_prev_combo = combo;
  if (!trigger)
    return;

  // Must be in main gameplay
  if (game_state != kGameState_8_MainGameplay)
    return;

  // Must have charges
  if (fs_charges == 0)
    return;

  // Movement type must be allowed (no morph, grapple, shinespark, etc.)
  if (!IsMovementTypeAllowed())
    return;

  // Spend charge and set cooldown
  fs_charges--;
  fs_cooldown = (fs_charges == 0) ? kFlashShift_EmptyCooldown : kFlashShift_Cooldown;
  fs_regen_timer = kFlashShift_RegenTime;

  // Zero all momentum
  samus_x_base_speed = 0;
  samus_x_base_subspeed = 0;
  samus_x_extra_run_speed = 0;
  samus_x_extra_run_subspeed = 0;
  samus_y_speed = 0;
  samus_y_subspeed = 0;

  // Set collision direction and step amount
  int32 step;
  if (right) {
    samus_collision_direction = 1;
    step = kFlashShift_StepSize << 16;  // positive = right
  } else {
    samus_collision_direction = 0;
    step = -(kFlashShift_StepSize << 16);  // negative = left
  }

  // Move in steps, stopping on wall collision
  for (int i = 0; i < kFlashShift_Steps; i++) {
    Samus_MoveRight_NoSolidColl(step);
    if (samus_collision_flag)
      break;
  }

  // Tell the game Samus didn't "move" this frame so the scroll handler
  // doesn't compute a huge absolute_moved_last_frame_x and fight our
  // camera snap.
  samus_prev_x_pos = samus_x_pos;
  samus_prev_x_subpos = samus_x_subpos;

  // Snap camera to center on Samus's new position and fully redraw the
  // tilemap in VRAM. This runs before RtlRunFrame, so MakeSnapshot
  // captures the corrected camera position AND VRAM tiles together.
  // The ROM then runs from this consistent state.
  {
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

  // Instantly face the shift direction and set a clean pose.
  // This breaks spin jumps into falling, and prevents the slow
  // turn-around animation when shifting opposite to current facing.
  {
    samus_pose_x_dir = right ? 0 : 4;
    bool airborne = (samus_movement_type == kMovementType_02_NormalJumping ||
                     samus_movement_type == kMovementType_03_SpinJumping ||
                     samus_movement_type == kMovementType_06_Falling ||
                     samus_movement_type == kMovementType_14_WallJumping ||
                     samus_movement_type == kMovementType_17_TurningAroundJumping ||
                     samus_movement_type == kMovementType_18_TurningAroundFalling);
    if (airborne) {
      samus_movement_type = kMovementType_06_Falling;
      samus_pose = right ? kPose_29_FaceR_Fall : kPose_2A_FaceL_Fall;
    } else {
      samus_movement_type = kMovementType_00_Standing;
      samus_pose = right ? kPose_01_FaceR_Normal : kPose_02_FaceL_Normal;
    }
    samus_anim_frame = 0;
  }

  // Post-shift effects
  samus_invincibility_timer = kFlashShift_Invincibility;
  samus_hurt_flash_counter = 3;
  fs_flash_timer = kFlashShift_FlashFrames;
  fs_freeze_timer = kFlashShift_FreezeFrames;

  // Shinespark charge SFX on channel 3, cut short after ~0.25s.
  // Silence the APU first so it restarts from the beginning even if
  // the same sound was already playing, then flush the queue and
  // reset the state machine before queuing the fresh sound.
  RtlApuWrite(APUI03, 0);
  sfx_cur[2] = 0;
  sfx_state[2] = 0;
  sfx_readpos[2] = sfx_writepos[2];
  QueueSfx3_Max9(0xC);
  fs_sfx_timer = kFlashShift_SfxDuration;
}
