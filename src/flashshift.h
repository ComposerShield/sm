#pragma once
#include "types.h"
#include <stdbool.h>

void FlashShift_Update(bool g_key_held);
bool FlashShift_IsEnabled(void);
void FlashShift_SetEnabled(bool enabled);
uint8 FlashShift_GetCharges(void);
