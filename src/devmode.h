#pragma once
#include "types.h"
#include <stdbool.h>

void DevMode_Toggle(void);
bool DevMode_IsOpen(void);
void DevMode_HandleInput(int key, bool pressed);
void DevMode_Render(uint8 *pixels, int width, int height, int pitch);
