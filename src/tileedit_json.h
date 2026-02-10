#pragma once
#include "types.h"
#include <stdbool.h>

struct CustomRoom;

bool TileEditJson_Save(const struct CustomRoom *room, const char *path);
bool TileEditJson_Load(struct CustomRoom *room, const char *path);
