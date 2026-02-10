#include "devmode.h"
#include "sm_rtl.h"
#include "variables.h"
#include "ida_types.h"
#include <string.h>
#include <SDL.h>

// 8x8 bitmap font for printable ASCII 32-126
static const uint8 kFont8x8[95][8] = {
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 ' '
  {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00}, // 33 '!'
  {0x6c,0x6c,0x00,0x00,0x00,0x00,0x00,0x00}, // 34 '"'
  {0x6c,0x6c,0xfe,0x6c,0xfe,0x6c,0x6c,0x00}, // 35 '#'
  {0x18,0x7e,0x06,0x3c,0x60,0x3e,0x18,0x00}, // 36 '$'
  {0x00,0x66,0x30,0x18,0x0c,0x66,0x00,0x00}, // 37 '%'
  {0x1c,0x36,0x1c,0x6e,0x3b,0x33,0x6e,0x00}, // 38 '&'
  {0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00}, // 39 '''
  {0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00}, // 40 '('
  {0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00}, // 41 ')'
  {0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00}, // 42 '*'
  {0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00}, // 43 '+'
  {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x0c}, // 44 ','
  {0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00}, // 45 '-'
  {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 '.'
  {0x60,0x30,0x18,0x0c,0x06,0x03,0x01,0x00}, // 47 '/'
  {0x3c,0x66,0x76,0x7e,0x6e,0x66,0x3c,0x00}, // 48 '0'
  {0x18,0x1c,0x18,0x18,0x18,0x18,0x7e,0x00}, // 49 '1'
  {0x3c,0x66,0x60,0x30,0x0c,0x06,0x7e,0x00}, // 50 '2'
  {0x3c,0x66,0x60,0x38,0x60,0x66,0x3c,0x00}, // 51 '3'
  {0x30,0x38,0x3c,0x36,0x7e,0x30,0x30,0x00}, // 52 '4'
  {0x7e,0x06,0x3e,0x60,0x60,0x66,0x3c,0x00}, // 53 '5'
  {0x38,0x0c,0x06,0x3e,0x66,0x66,0x3c,0x00}, // 54 '6'
  {0x7e,0x60,0x30,0x18,0x0c,0x0c,0x0c,0x00}, // 55 '7'
  {0x3c,0x66,0x66,0x3c,0x66,0x66,0x3c,0x00}, // 56 '8'
  {0x3c,0x66,0x66,0x7c,0x60,0x30,0x1c,0x00}, // 57 '9'
  {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00}, // 58 ':'
  {0x00,0x18,0x18,0x00,0x18,0x18,0x0c,0x00}, // 59 ';'
  {0x30,0x18,0x0c,0x06,0x0c,0x18,0x30,0x00}, // 60 '<'
  {0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00}, // 61 '='
  {0x0c,0x18,0x30,0x60,0x30,0x18,0x0c,0x00}, // 62 '>'
  {0x3c,0x66,0x30,0x18,0x18,0x00,0x18,0x00}, // 63 '?'
  {0x3c,0x66,0x76,0x56,0x76,0x06,0x3c,0x00}, // 64 '@'
  {0x18,0x3c,0x66,0x66,0x7e,0x66,0x66,0x00}, // 65 'A'
  {0x3e,0x66,0x66,0x3e,0x66,0x66,0x3e,0x00}, // 66 'B'
  {0x3c,0x66,0x06,0x06,0x06,0x66,0x3c,0x00}, // 67 'C'
  {0x1e,0x36,0x66,0x66,0x66,0x36,0x1e,0x00}, // 68 'D'
  {0x7e,0x06,0x06,0x3e,0x06,0x06,0x7e,0x00}, // 69 'E'
  {0x7e,0x06,0x06,0x3e,0x06,0x06,0x06,0x00}, // 70 'F'
  {0x3c,0x66,0x06,0x76,0x66,0x66,0x3c,0x00}, // 71 'G'
  {0x66,0x66,0x66,0x7e,0x66,0x66,0x66,0x00}, // 72 'H'
  {0x3c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00}, // 73 'I'
  {0x78,0x30,0x30,0x30,0x30,0x36,0x1c,0x00}, // 74 'J'
  {0x66,0x36,0x1e,0x0e,0x1e,0x36,0x66,0x00}, // 75 'K'
  {0x06,0x06,0x06,0x06,0x06,0x06,0x7e,0x00}, // 76 'L'
  {0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00}, // 77 'M'
  {0x66,0x6e,0x7e,0x76,0x66,0x66,0x66,0x00}, // 78 'N'
  {0x3c,0x66,0x66,0x66,0x66,0x66,0x3c,0x00}, // 79 'O'
  {0x3e,0x66,0x66,0x3e,0x06,0x06,0x06,0x00}, // 80 'P'
  {0x3c,0x66,0x66,0x66,0x6e,0x36,0x5c,0x00}, // 81 'Q'
  {0x3e,0x66,0x66,0x3e,0x36,0x66,0x66,0x00}, // 82 'R'
  {0x3c,0x66,0x06,0x3c,0x60,0x66,0x3c,0x00}, // 83 'S'
  {0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 84 'T'
  {0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0x00}, // 85 'U'
  {0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x00}, // 86 'V'
  {0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00}, // 87 'W'
  {0x66,0x66,0x3c,0x18,0x3c,0x66,0x66,0x00}, // 88 'X'
  {0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00}, // 89 'Y'
  {0x7e,0x60,0x30,0x18,0x0c,0x06,0x7e,0x00}, // 90 'Z'
  {0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00}, // 91 '['
  {0x01,0x03,0x06,0x0c,0x18,0x30,0x60,0x00}, // 92 '\'
  {0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0x00}, // 93 ']'
  {0x18,0x3c,0x66,0x00,0x00,0x00,0x00,0x00}, // 94 '^'
  {0x00,0x00,0x00,0x00,0x00,0x00,0x7e,0x00}, // 95 '_'
  {0x0c,0x18,0x00,0x00,0x00,0x00,0x00,0x00}, // 96 '`'
  {0x00,0x00,0x3c,0x60,0x7c,0x66,0x7c,0x00}, // 97 'a'
  {0x06,0x06,0x3e,0x66,0x66,0x66,0x3e,0x00}, // 98 'b'
  {0x00,0x00,0x3c,0x06,0x06,0x06,0x3c,0x00}, // 99 'c'
  {0x60,0x60,0x7c,0x66,0x66,0x66,0x7c,0x00}, // 100 'd'
  {0x00,0x00,0x3c,0x66,0x7e,0x06,0x3c,0x00}, // 101 'e'
  {0x38,0x0c,0x0c,0x3e,0x0c,0x0c,0x0c,0x00}, // 102 'f'
  {0x00,0x00,0x7c,0x66,0x7c,0x60,0x3c,0x00}, // 103 'g'
  {0x06,0x06,0x3e,0x66,0x66,0x66,0x66,0x00}, // 104 'h'
  {0x18,0x00,0x1c,0x18,0x18,0x18,0x3c,0x00}, // 105 'i'
  {0x30,0x00,0x38,0x30,0x30,0x30,0x1e,0x00}, // 106 'j'
  {0x06,0x06,0x36,0x1e,0x1e,0x36,0x66,0x00}, // 107 'k'
  {0x1c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00}, // 108 'l'
  {0x00,0x00,0x37,0x7f,0x6b,0x63,0x63,0x00}, // 109 'm'
  {0x00,0x00,0x3e,0x66,0x66,0x66,0x66,0x00}, // 110 'n'
  {0x00,0x00,0x3c,0x66,0x66,0x66,0x3c,0x00}, // 111 'o'
  {0x00,0x00,0x3e,0x66,0x3e,0x06,0x06,0x00}, // 112 'p'
  {0x00,0x00,0x7c,0x66,0x7c,0x60,0x60,0x00}, // 113 'q'
  {0x00,0x00,0x36,0x1e,0x06,0x06,0x06,0x00}, // 114 'r'
  {0x00,0x00,0x7c,0x06,0x3c,0x60,0x3e,0x00}, // 115 's'
  {0x0c,0x0c,0x3e,0x0c,0x0c,0x0c,0x38,0x00}, // 116 't'
  {0x00,0x00,0x66,0x66,0x66,0x66,0x7c,0x00}, // 117 'u'
  {0x00,0x00,0x66,0x66,0x66,0x3c,0x18,0x00}, // 118 'v'
  {0x00,0x00,0x63,0x6b,0x7f,0x7f,0x36,0x00}, // 119 'w'
  {0x00,0x00,0x66,0x3c,0x18,0x3c,0x66,0x00}, // 120 'x'
  {0x00,0x00,0x66,0x66,0x7c,0x60,0x3c,0x00}, // 121 'y'
  {0x00,0x00,0x7e,0x30,0x18,0x0c,0x7e,0x00}, // 122 'z'
  {0x30,0x18,0x18,0x0c,0x18,0x18,0x30,0x00}, // 123 '{'
  {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 124 '|'
  {0x0c,0x18,0x18,0x30,0x18,0x18,0x0c,0x00}, // 125 '}'
  {0x00,0x00,0x4e,0x79,0x00,0x00,0x00,0x00}, // 126 '~'
};

// Room table: area_index, load_station_index, name
typedef struct DevModeRoom {
  const char *name;
  uint8 area;
  uint8 station;
} DevModeRoom;

static const DevModeRoom kRooms[] = {
  {"Crateria - Landing Site",   0, 0},
  {"Crateria - Parlor",         0, 1},
  {"Brinstar - Green Save",     1, 0},
  {"Brinstar - Red Tower",      1, 1},
  {"Brinstar - Kraid Save",     1, 2},
  {"Norfair - Business Center", 2, 0},
  {"Norfair - Bubble Mountain", 2, 1},
  {"Norfair - Crocomire",       2, 2},
  {"Norfair - Lower Entry",     2, 3},
  {"Wrecked Ship - Save",       3, 0},
  {"Maridia - Main Save",       4, 0},
  {"Maridia - Aqueduct",        4, 1},
  {"Tourian - Entrance",        5, 0},
  {"Ceres - Start",             6, 0},
};
#define kNumRooms (int)(sizeof(kRooms) / sizeof(kRooms[0]))

// Item bitmask definitions (Super Metroid equipped_items bits)
enum {
  kDevItem_VariaSuit    = 0x0001,
  kDevItem_SpringBall   = 0x0002,
  kDevItem_MorphBall    = 0x0004,
  kDevItem_ScrewAttack  = 0x0008,
  kDevItem_GravitySuit  = 0x0020,
  kDevItem_HiJump       = 0x0100,
  kDevItem_SpaceJump    = 0x0200,
  kDevItem_Bombs        = 0x1000,
  kDevItem_SpeedBooster = 0x2000,
  kDevItem_Grapple      = 0x4000,
  kDevItem_XRay         = 0x8000,
};

// Beam bitmask definitions (Super Metroid equipped_beams bits)
enum {
  kDevBeam_Wave   = 0x0001,
  kDevBeam_Ice    = 0x0002,
  kDevBeam_Spazer = 0x0004,
  kDevBeam_Plasma = 0x0008,
  kDevBeam_Charge = 0x1000,
};

typedef struct DevItemDef {
  const char *name;
  uint16 mask;
} DevItemDef;

static const DevItemDef kItems[] = {
  {"Morph Ball",    kDevItem_MorphBall},
  {"Varia Suit",    kDevItem_VariaSuit},
  {"Gravity Suit",  kDevItem_GravitySuit},
  {"Hi-Jump",       kDevItem_HiJump},
  {"Space Jump",    kDevItem_SpaceJump},
  {"Speed Boost",   kDevItem_SpeedBooster},
  {"Screw Attack",  kDevItem_ScrewAttack},
  {"Spring Ball",   kDevItem_SpringBall},
  {"Grapple",       kDevItem_Grapple},
  {"X-Ray",         kDevItem_XRay},
  {"Bombs",         kDevItem_Bombs},
};
#define kNumItems (int)(sizeof(kItems) / sizeof(kItems[0]))

static const DevItemDef kBeams[] = {
  {"Charge", kDevBeam_Charge},
  {"Wave",   kDevBeam_Wave},
  {"Ice",    kDevBeam_Ice},
  {"Spazer", kDevBeam_Spazer},
  {"Plasma", kDevBeam_Plasma},
};
#define kNumBeams (int)(sizeof(kBeams) / sizeof(kBeams[0]))

// Menu sections
enum {
  kSection_Rooms,
  kSection_Health,
  kSection_Missiles,
  kSection_Supers,
  kSection_PBombs,
  kSection_Items,
  kSection_Beams,
  kSection_Count,
};

// Menu state
static struct {
  bool open;
  int section;       // which section is focused
  int room_sel;      // selected room index
  int item_cursor;   // cursor within items section
  int beam_cursor;   // cursor within beams section
  uint16 health, max_health;
  uint16 missiles, max_missiles;
  uint16 supers, max_supers;
  uint16 pbombs, max_pbombs;
  uint16 items_mask;
  uint16 beams_mask;
} menu = {
  .open = false,
  .section = kSection_Rooms,
  .room_sel = 0,
  .item_cursor = 0,
  .beam_cursor = 0,
  .health = 599, .max_health = 599,
  .missiles = 5, .max_missiles = 5,
  .supers = 5, .max_supers = 5,
  .pbombs = 5, .max_pbombs = 5,
  .items_mask = kDevItem_MorphBall,
  .beams_mask = 0,
};

// Drawing helpers — all bounds-checked against g_render_w / g_render_h
static int g_render_w, g_render_h;

static void DrawChar(uint8 *pixels, int pitch, int x, int y, char c, uint32 color) {
  if (c < 32 || c > 126) return;
  if (x < 0 || x + 8 > g_render_w || y < 0 || y + 8 > g_render_h) return;
  const uint8 *glyph = kFont8x8[c - 32];
  uint8 *dst = pixels + y * pitch + x * 4;
  for (int row = 0; row < 8; row++, dst += pitch) {
    uint8 bits = glyph[row];
    for (int col = 0; col < 8; col++) {
      if (bits & (1 << col))
        ((uint32 *)dst)[col] = color;
    }
  }
}

static void DrawString(uint8 *pixels, int pitch, int x, int y, const char *s, uint32 color) {
  for (; *s; s++, x += 8)
    DrawChar(pixels, pitch, x, y, *s, color);
}

static void DrawRect(uint8 *pixels, int pitch, int x, int y, int w, int h, uint32 color) {
  for (int row = 0; row < h; row++) {
    int py = y + row;
    if (py < 0 || py >= g_render_h) continue;
    uint32 *dst = (uint32 *)(pixels + py * pitch + x * 4);
    for (int col = 0; col < w; col++) {
      int px = x + col;
      if (px >= 0 && px < g_render_w)
        dst[col] = color;
    }
  }
}

static void DrawNumber(uint8 *pixels, int pitch, int x, int y, int n, int digits, uint32 color) {
  char buf[8];
  int i;
  for (i = digits - 1; i >= 0; i--) {
    buf[i] = '0' + (n % 10);
    n /= 10;
  }
  buf[digits] = 0;
  DrawString(pixels, pitch, x, y, buf, color);
}

void DevMode_Toggle(void) {
  menu.open = !menu.open;
}

bool DevMode_IsOpen(void) {
  return menu.open;
}

static void AdjustValue(uint16 *val, uint16 *max_val, int delta, uint16 limit) {
  int v = *val + delta;
  if (v < 0) v = 0;
  if (v > limit) v = limit;
  *val = (uint16)v;
  if (*max_val < *val) *max_val = *val;

  int mv = *max_val + delta;
  if (mv < *val) mv = *val;
  if (mv > limit) mv = limit;
  *max_val = (uint16)mv;
}

static void AdjustMaxValue(uint16 *val, uint16 *max_val, int delta, uint16 limit) {
  int mv = *max_val + delta;
  if (mv < 1) mv = 1;
  if (mv > limit) mv = limit;
  *max_val = (uint16)mv;
  if (*val > *max_val) *val = *max_val;
}

void DevMode_HandleInput(int key, bool pressed) {
  if (!pressed) return;

  switch (key) {
  case SDLK_ESCAPE:
  case SDLK_BACKQUOTE:
    menu.open = false;
    return;
  case SDLK_w:
    // Execute warp
    if (game_state >= kGameState_6_LoadingGameData) {
      const DevModeRoom *r = &kRooms[menu.room_sel];
      RtlDevModeWarp(r->area, r->station,
                      menu.items_mask, menu.beams_mask,
                      menu.health, menu.max_health,
                      menu.missiles, menu.max_missiles,
                      menu.supers, menu.max_supers,
                      menu.pbombs, menu.max_pbombs);
      menu.open = false;
    }
    return;
  case SDLK_UP:
    if (menu.section == kSection_Rooms) {
      if (menu.room_sel > 0) menu.room_sel--;
    } else if (menu.section == kSection_Items) {
      if (menu.item_cursor > 0) menu.item_cursor--;
      else menu.section--;
    } else if (menu.section == kSection_Beams) {
      if (menu.beam_cursor > 0) menu.beam_cursor--;
      else menu.section--;
    } else {
      if (menu.section > 0) menu.section--;
    }
    return;
  case SDLK_DOWN:
    if (menu.section == kSection_Rooms) {
      if (menu.room_sel < kNumRooms - 1) menu.room_sel++;
      else menu.section++;
    } else if (menu.section == kSection_Items) {
      if (menu.item_cursor < kNumItems - 1) menu.item_cursor++;
      else menu.section++;
    } else if (menu.section == kSection_Beams) {
      if (menu.beam_cursor < kNumBeams - 1) menu.beam_cursor++;
      else { /* already at bottom */ }
    } else {
      if (menu.section < kSection_Count - 1) menu.section++;
    }
    return;
  case SDLK_LEFT:
    switch (menu.section) {
    case kSection_Rooms:
      if (menu.room_sel > 0) menu.room_sel--;
      break;
    case kSection_Health:
      AdjustMaxValue(&menu.health, &menu.max_health, -100, 2099);
      break;
    case kSection_Missiles:
      AdjustMaxValue(&menu.missiles, &menu.max_missiles, -5, 230);
      break;
    case kSection_Supers:
      AdjustMaxValue(&menu.supers, &menu.max_supers, -5, 50);
      break;
    case kSection_PBombs:
      AdjustMaxValue(&menu.pbombs, &menu.max_pbombs, -5, 50);
      break;
    }
    return;
  case SDLK_RIGHT:
    switch (menu.section) {
    case kSection_Rooms:
      if (menu.room_sel < kNumRooms - 1) menu.room_sel++;
      break;
    case kSection_Health:
      AdjustMaxValue(&menu.health, &menu.max_health, 100, 2099);
      break;
    case kSection_Missiles:
      AdjustMaxValue(&menu.missiles, &menu.max_missiles, 5, 230);
      break;
    case kSection_Supers:
      AdjustMaxValue(&menu.supers, &menu.max_supers, 5, 50);
      break;
    case kSection_PBombs:
      AdjustMaxValue(&menu.pbombs, &menu.max_pbombs, 5, 50);
      break;
    }
    return;
  case SDLK_RETURN:
  case SDLK_SPACE:
    if (menu.section == kSection_Items) {
      menu.items_mask ^= kItems[menu.item_cursor].mask;
    } else if (menu.section == kSection_Beams) {
      menu.beams_mask ^= kBeams[menu.beam_cursor].mask;
    }
    return;
  }
}

void DevMode_Render(uint8 *pixels, int width, int height, int pitch) {
  g_render_w = width;
  g_render_h = height;

  // Menu dimensions — must fit in 240px height
  int menu_w = 230;
  int menu_h = IntMin(height - 4, 234);
  int mx = (width - menu_w) / 2;
  int my = (height - menu_h) / 2;
  if (mx < 0) mx = 0;
  if (my < 0) my = 0;

  // Dark overlay on entire screen
  for (int row = 0; row < height; row++) {
    uint32 *dst = (uint32 *)(pixels + row * pitch);
    for (int col = 0; col < width; col++) {
      uint32 c = dst[col];
      dst[col] = ((c >> 1) & 0x7f7f7f);
    }
  }

  // Menu background
  DrawRect(pixels, pitch, mx, my, menu_w, menu_h, 0x101830);

  // Border
  for (int col = 0; col < menu_w; col++) {
    ((uint32 *)(pixels + my * pitch))[mx + col] = 0x4060a0;
    ((uint32 *)(pixels + (my + menu_h - 1) * pitch))[mx + col] = 0x4060a0;
  }
  for (int row = 0; row < menu_h; row++) {
    ((uint32 *)(pixels + (my + row) * pitch))[mx] = 0x4060a0;
    ((uint32 *)(pixels + (my + row) * pitch))[mx + menu_w - 1] = 0x4060a0;
  }

  int cx = mx + 6;
  int cy = my + 4;
  uint32 white = 0xffffff;
  uint32 gray = 0x808080;
  uint32 yellow = 0xffff00;
  uint32 cyan = 0x00ffff;
  uint32 green = 0x40ff40;

  DrawString(pixels, pitch, cx, cy, "DEV MODE", yellow);
  DrawString(pixels, pitch, mx + menu_w - 14, cy, "~", gray);
  cy += 10;

  // Rooms section — scrollable, 4 visible
  DrawString(pixels, pitch, cx, cy, "ROOM:", cyan);
  cy += 9;

  int visible_rooms = 4;
  int scroll_start = 0;
  if (menu.room_sel >= visible_rooms)
    scroll_start = menu.room_sel - visible_rooms + 1;
  if (scroll_start > kNumRooms - visible_rooms)
    scroll_start = kNumRooms - visible_rooms;
  if (scroll_start < 0)
    scroll_start = 0;

  for (int i = scroll_start; i < kNumRooms && i < scroll_start + visible_rooms; i++) {
    bool selected = (i == menu.room_sel);
    uint32 color = selected && menu.section == kSection_Rooms ? yellow : (selected ? white : gray);
    DrawString(pixels, pitch, cx, cy, selected ? ">" : " ", color);
    DrawString(pixels, pitch, cx + 8, cy, kRooms[i].name, color);
    cy += 9;
  }
  cy += 2;

  // Loadout — compact, 2 per row
  {
    uint32 c1 = menu.section == kSection_Health ? yellow : white;
    uint32 c2 = menu.section == kSection_Missiles ? yellow : white;
    DrawString(pixels, pitch, cx, cy, "HP:", c1);
    DrawNumber(pixels, pitch, cx + 24, cy, menu.health, 4, c1);
    DrawString(pixels, pitch, cx + 56, cy, "/", c1);
    DrawNumber(pixels, pitch, cx + 64, cy, menu.max_health, 4, c1);
    DrawString(pixels, pitch, cx + 104, cy, "Mis:", c2);
    DrawNumber(pixels, pitch, cx + 136, cy, menu.missiles, 3, c2);
    DrawString(pixels, pitch, cx + 160, cy, "/", c2);
    DrawNumber(pixels, pitch, cx + 168, cy, menu.max_missiles, 3, c2);
    cy += 9;
  }
  {
    uint32 c1 = menu.section == kSection_Supers ? yellow : white;
    uint32 c2 = menu.section == kSection_PBombs ? yellow : white;
    DrawString(pixels, pitch, cx, cy, "Sup:", c1);
    DrawNumber(pixels, pitch, cx + 32, cy, menu.supers, 3, c1);
    DrawString(pixels, pitch, cx + 56, cy, "/", c1);
    DrawNumber(pixels, pitch, cx + 64, cy, menu.max_supers, 3, c1);
    DrawString(pixels, pitch, cx + 104, cy, "PBs:", c2);
    DrawNumber(pixels, pitch, cx + 136, cy, menu.pbombs, 3, c2);
    DrawString(pixels, pitch, cx + 160, cy, "/", c2);
    DrawNumber(pixels, pitch, cx + 168, cy, menu.max_pbombs, 3, c2);
    cy += 9;
  }
  cy += 2;

  // Items — 2 per row
  DrawString(pixels, pitch, cx, cy, "ITEMS:", cyan);
  cy += 9;
  for (int i = 0; i < kNumItems; i += 2) {
    for (int col = 0; col < 2 && i + col < kNumItems; col++) {
      int idx = i + col;
      bool on = (menu.items_mask & kItems[idx].mask) != 0;
      bool sel = (menu.section == kSection_Items && menu.item_cursor == idx);
      uint32 c = sel ? yellow : white;
      int xoff = cx + col * 112;
      char buf[16];
      buf[0] = '['; buf[1] = on ? 'x' : ' '; buf[2] = ']'; buf[3] = ' '; buf[4] = 0;
      DrawString(pixels, pitch, xoff, cy, buf, c);
      DrawString(pixels, pitch, xoff + 32, cy, kItems[idx].name, c);
    }
    cy += 9;
  }
  cy += 2;

  // Beams — all on one row
  DrawString(pixels, pitch, cx, cy, "BEAMS:", cyan);
  cy += 9;
  for (int i = 0; i < kNumBeams; i += 3) {
    for (int col = 0; col < 3 && i + col < kNumBeams; col++) {
      int idx = i + col;
      bool on = (menu.beams_mask & kBeams[idx].mask) != 0;
      bool sel = (menu.section == kSection_Beams && menu.beam_cursor == idx);
      uint32 c = sel ? yellow : white;
      int xoff = cx + col * 72;
      char buf[16];
      buf[0] = '['; buf[1] = on ? 'x' : ' '; buf[2] = ']'; buf[3] = ' '; buf[4] = 0;
      DrawString(pixels, pitch, xoff, cy, buf, c);
      DrawString(pixels, pitch, xoff + 32, cy, kBeams[idx].name, c);
    }
    cy += 9;
  }
  cy += 2;

  // Footer
  DrawString(pixels, pitch, cx, cy, "[W] WARP  [ESC] CANCEL", green);
  if (game_state < kGameState_6_LoadingGameData)
    DrawString(pixels, pitch, cx, cy + 9, "Start game first!", 0xff4040);
}
