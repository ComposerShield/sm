#pragma once
#include "types.h"
#include <stdbool.h>

struct Snes;

// Return codes from editor main loop
enum {
  kTileEditResult_Quit = 0,
  kTileEditResult_Playtest = 1,
};

// Main editor loop — takes over from main() when --editor is passed
// Returns kTileEditResult_Quit or kTileEditResult_Playtest
int TileEdit_MainLoop(struct Snes *snes);

// After playtesting, inject custom room data into game state
// Call after game_state reaches kGameState_8_MainGameplay
void TileEdit_InjectCustomRoom(void);

// Get the area/station indices for playtesting warp
void TileEdit_GetPlaytestWarp(uint8 *area, uint8 *station);

// Check if there's a pending custom room injection (called each frame during playtest)
bool TileEdit_HasPendingPlaytest(void);
