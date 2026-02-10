#include "tileedit.h"
#include "tileedit_json.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include "variables.h"
#include "ida_types.h"
#include "funcs.h"
#include "snes/snes.h"
#include "snes/ppu.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

// ---------------------------------------------------------------------------
// 8x8 bitmap font (same as devmode)
// ---------------------------------------------------------------------------
static const uint8 kFont8x8[95][8] = {
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00},
  {0x6c,0x6c,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x6c,0x6c,0xfe,0x6c,0xfe,0x6c,0x6c,0x00},
  {0x18,0x7e,0x06,0x3c,0x60,0x3e,0x18,0x00},
  {0x00,0x66,0x30,0x18,0x0c,0x66,0x00,0x00},
  {0x1c,0x36,0x1c,0x6e,0x3b,0x33,0x6e,0x00},
  {0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00},
  {0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00},
  {0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00},
  {0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x0c},
  {0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00},
  {0x60,0x30,0x18,0x0c,0x06,0x03,0x01,0x00},
  {0x3c,0x66,0x76,0x7e,0x6e,0x66,0x3c,0x00},
  {0x18,0x1c,0x18,0x18,0x18,0x18,0x7e,0x00},
  {0x3c,0x66,0x60,0x30,0x0c,0x06,0x7e,0x00},
  {0x3c,0x66,0x60,0x38,0x60,0x66,0x3c,0x00},
  {0x30,0x38,0x3c,0x36,0x7e,0x30,0x30,0x00},
  {0x7e,0x06,0x3e,0x60,0x60,0x66,0x3c,0x00},
  {0x38,0x0c,0x06,0x3e,0x66,0x66,0x3c,0x00},
  {0x7e,0x60,0x30,0x18,0x0c,0x0c,0x0c,0x00},
  {0x3c,0x66,0x66,0x3c,0x66,0x66,0x3c,0x00},
  {0x3c,0x66,0x66,0x7c,0x60,0x30,0x1c,0x00},
  {0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00},
  {0x00,0x18,0x18,0x00,0x18,0x18,0x0c,0x00},
  {0x30,0x18,0x0c,0x06,0x0c,0x18,0x30,0x00},
  {0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00},
  {0x0c,0x18,0x30,0x60,0x30,0x18,0x0c,0x00},
  {0x3c,0x66,0x30,0x18,0x18,0x00,0x18,0x00},
  {0x3c,0x66,0x76,0x56,0x76,0x06,0x3c,0x00},
  {0x18,0x3c,0x66,0x66,0x7e,0x66,0x66,0x00},
  {0x3e,0x66,0x66,0x3e,0x66,0x66,0x3e,0x00},
  {0x3c,0x66,0x06,0x06,0x06,0x66,0x3c,0x00},
  {0x1e,0x36,0x66,0x66,0x66,0x36,0x1e,0x00},
  {0x7e,0x06,0x06,0x3e,0x06,0x06,0x7e,0x00},
  {0x7e,0x06,0x06,0x3e,0x06,0x06,0x06,0x00},
  {0x3c,0x66,0x06,0x76,0x66,0x66,0x3c,0x00},
  {0x66,0x66,0x66,0x7e,0x66,0x66,0x66,0x00},
  {0x3c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00},
  {0x78,0x30,0x30,0x30,0x30,0x36,0x1c,0x00},
  {0x66,0x36,0x1e,0x0e,0x1e,0x36,0x66,0x00},
  {0x06,0x06,0x06,0x06,0x06,0x06,0x7e,0x00},
  {0x63,0x77,0x7f,0x6b,0x63,0x63,0x63,0x00},
  {0x66,0x6e,0x7e,0x76,0x66,0x66,0x66,0x00},
  {0x3c,0x66,0x66,0x66,0x66,0x66,0x3c,0x00},
  {0x3e,0x66,0x66,0x3e,0x06,0x06,0x06,0x00},
  {0x3c,0x66,0x66,0x66,0x6e,0x36,0x5c,0x00},
  {0x3e,0x66,0x66,0x3e,0x36,0x66,0x66,0x00},
  {0x3c,0x66,0x06,0x3c,0x60,0x66,0x3c,0x00},
  {0x7e,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
  {0x66,0x66,0x66,0x66,0x66,0x66,0x3c,0x00},
  {0x66,0x66,0x66,0x66,0x66,0x3c,0x18,0x00},
  {0x63,0x63,0x63,0x6b,0x7f,0x77,0x63,0x00},
  {0x66,0x66,0x3c,0x18,0x3c,0x66,0x66,0x00},
  {0x66,0x66,0x66,0x3c,0x18,0x18,0x18,0x00},
  {0x7e,0x60,0x30,0x18,0x0c,0x06,0x7e,0x00},
  {0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00},
  {0x01,0x03,0x06,0x0c,0x18,0x30,0x60,0x00},
  {0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0x00},
  {0x18,0x3c,0x66,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x00,0x00,0x00,0x00,0x7e,0x00},
  {0x0c,0x18,0x00,0x00,0x00,0x00,0x00,0x00},
  {0x00,0x00,0x3c,0x60,0x7c,0x66,0x7c,0x00},
  {0x06,0x06,0x3e,0x66,0x66,0x66,0x3e,0x00},
  {0x00,0x00,0x3c,0x06,0x06,0x06,0x3c,0x00},
  {0x60,0x60,0x7c,0x66,0x66,0x66,0x7c,0x00},
  {0x00,0x00,0x3c,0x66,0x7e,0x06,0x3c,0x00},
  {0x38,0x0c,0x0c,0x3e,0x0c,0x0c,0x0c,0x00},
  {0x00,0x00,0x7c,0x66,0x7c,0x60,0x3c,0x00},
  {0x06,0x06,0x3e,0x66,0x66,0x66,0x66,0x00},
  {0x18,0x00,0x1c,0x18,0x18,0x18,0x3c,0x00},
  {0x30,0x00,0x38,0x30,0x30,0x30,0x1e,0x00},
  {0x06,0x06,0x36,0x1e,0x1e,0x36,0x66,0x00},
  {0x1c,0x18,0x18,0x18,0x18,0x18,0x3c,0x00},
  {0x00,0x00,0x37,0x7f,0x6b,0x63,0x63,0x00},
  {0x00,0x00,0x3e,0x66,0x66,0x66,0x66,0x00},
  {0x00,0x00,0x3c,0x66,0x66,0x66,0x3c,0x00},
  {0x00,0x00,0x3e,0x66,0x3e,0x06,0x06,0x00},
  {0x00,0x00,0x7c,0x66,0x7c,0x60,0x60,0x00},
  {0x00,0x00,0x36,0x1e,0x06,0x06,0x06,0x00},
  {0x00,0x00,0x7c,0x06,0x3c,0x60,0x3e,0x00},
  {0x0c,0x0c,0x3e,0x0c,0x0c,0x0c,0x38,0x00},
  {0x00,0x00,0x66,0x66,0x66,0x66,0x7c,0x00},
  {0x00,0x00,0x66,0x66,0x66,0x3c,0x18,0x00},
  {0x00,0x00,0x63,0x6b,0x7f,0x7f,0x36,0x00},
  {0x00,0x00,0x66,0x3c,0x18,0x3c,0x66,0x00},
  {0x00,0x00,0x66,0x66,0x7c,0x60,0x3c,0x00},
  {0x00,0x00,0x7e,0x30,0x18,0x0c,0x7e,0x00},
  {0x30,0x18,0x18,0x0c,0x18,0x18,0x30,0x00},
  {0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00},
  {0x0c,0x18,0x18,0x30,0x18,0x18,0x0c,0x00},
};

// ---------------------------------------------------------------------------
// Drawing helpers (operates on ARGB8888 pixel buffer)
// ---------------------------------------------------------------------------
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

static void DrawRectAlpha(uint8 *pixels, int pitch, int x, int y, int w, int h, uint32 color, uint8 alpha) {
  uint32 sr = (color >> 16) & 0xff, sg = (color >> 8) & 0xff, sb = color & 0xff;
  for (int row = 0; row < h; row++) {
    int py = y + row;
    if (py < 0 || py >= g_render_h) continue;
    uint32 *dst = (uint32 *)(pixels + py * pitch + x * 4);
    for (int col = 0; col < w; col++) {
      int px = x + col;
      if (px >= 0 && px < g_render_w) {
        uint32 d = dst[col];
        uint32 dr = (d >> 16) & 0xff, dg = (d >> 8) & 0xff, db = d & 0xff;
        dr = (sr * alpha + dr * (255 - alpha)) / 255;
        dg = (sg * alpha + dg * (255 - alpha)) / 255;
        db = (sb * alpha + db * (255 - alpha)) / 255;
        dst[col] = 0xff000000 | (dr << 16) | (dg << 8) | db;
      }
    }
  }
}

static void DrawRectOutline(uint8 *pixels, int pitch, int x, int y, int w, int h, uint32 color) {
  DrawRect(pixels, pitch, x, y, w, 1, color);
  DrawRect(pixels, pitch, x, y + h - 1, w, 1, color);
  DrawRect(pixels, pitch, x, y, 1, h, color);
  DrawRect(pixels, pitch, x + w - 1, y, 1, h, color);
}

// ---------------------------------------------------------------------------
// Data model
// ---------------------------------------------------------------------------
typedef struct CustomRoom {
  char name[64];
  int width_scrolls, height_scrolls;
  uint16 *blocks;
  uint8 *bts;
  int tileset_index;
  uint16 samus_start_x, samus_start_y;
  bool dirty;
  char filename[256];
} CustomRoom;

static int RoomWidthBlocks(const CustomRoom *r) { return r->width_scrolls * 16; }
static int RoomHeightBlocks(const CustomRoom *r) { return r->height_scrolls * 16; }
static int RoomTotalBlocks(const CustomRoom *r) { return RoomWidthBlocks(r) * RoomHeightBlocks(r); }

static void CustomRoom_Init(CustomRoom *r, int ws, int hs) {
  memset(r, 0, sizeof(*r));
  snprintf(r->name, sizeof(r->name), "Untitled");
  r->width_scrolls = ws;
  r->height_scrolls = hs;
  int total = RoomTotalBlocks(r);
  r->blocks = (uint16 *)calloc(total, sizeof(uint16));
  r->bts = (uint8 *)calloc(total, 1);
  r->tileset_index = 0;
  r->samus_start_x = 128;
  r->samus_start_y = 160;
  r->dirty = false;
  r->filename[0] = 0;
}

static void CustomRoom_Free(CustomRoom *r) {
  free(r->blocks);
  free(r->bts);
  r->blocks = NULL;
  r->bts = NULL;
}

// ---------------------------------------------------------------------------
// Tileset table: named tilesets with graphics_set index
// ---------------------------------------------------------------------------
typedef struct TilesetEntry {
  const char *name;
  int graphics_set;
} TilesetEntry;

static const TilesetEntry kTilesets[] = {
  {"Crateria Surface",      0},
  {"Crateria Underground",  2},
  {"Brinstar Green",        4},
  {"Brinstar Red",          5},
  {"Brinstar Kraid",        6},
  {"Norfair Heated",        9},
  {"Norfair Bubble",       10},
  {"Norfair Ridley",       12},
  {"Wrecked Ship",         15},
  {"Maridia Sandy",        18},
  {"Maridia Watery",       19},
  {"Tourian",              24},
  {"Ceres",                27},
};
enum { kNumTilesets = sizeof(kTilesets) / sizeof(kTilesets[0]) };

// Tileset index in kTilesets[] to area/station for playtesting
static const uint8 kTilesetToArea[] =    {0,0,1,1,1,2,2,2,3,4,4,5,6};
static const uint8 kTilesetToStation[] = {0,1,0,1,0,0,0,0,0,0,0,0,0};

// ---------------------------------------------------------------------------
// Block type definitions (upper 4 bits of block word)
// ---------------------------------------------------------------------------
enum {
  kBlockType_Air     = 0x0,
  kBlockType_Slope   = 0x1,
  kBlockType_Solid   = 0x8,
  kBlockType_Door    = 0x9,
  kBlockType_Spike   = 0xA,
  kBlockType_Bomb    = 0xF,
};

static const struct { const char *name; int type; uint32 color; } kBlockTypes[] = {
  {"Air",   kBlockType_Air,   0x00000000},
  {"Solid", kBlockType_Solid, 0x4040ff},
  {"Spike", kBlockType_Spike, 0xff4040},
  {"Bomb",  kBlockType_Bomb,  0xffff40},
};
enum { kNumBlockTypes = sizeof(kBlockTypes) / sizeof(kBlockTypes[0]) };

// ---------------------------------------------------------------------------
// Pre-rendered tile sheet (1024 blocks x 16x16 ARGB)
// ---------------------------------------------------------------------------
enum { kTileSheetCols = 16, kMaxBlocks = 1024 };
static uint32 g_tilesheet[kMaxBlocks * 16 * 16];

static Snes *g_ed_snes;

static void RenderOneTile8x8(uint32 *out, int out_stride,
                              uint16 tilemap_word, Ppu *ppu) {
  int tile_num = tilemap_word & 0x3FF;
  int palette  = (tilemap_word >> 10) & 7;
  bool hflip   = (tilemap_word >> 14) & 1;
  bool vflip   = (tilemap_word >> 15) & 1;

  // BG1 tile address (base for tileset tiles)
  uint16 tile_adr = ppu->bgLayer[0].tileAdr;
  // Each 4bpp tile = 16 words in VRAM
  int vram_offset = tile_adr + tile_num * 16;

  for (int py = 0; py < 8; py++) {
    int sy = vflip ? (7 - py) : py;
    // 4bpp: 2 words per row
    // word0 = bitplanes 0,1 at offset+row, word1 = bitplanes 2,3 at offset+row+8
    uint16 w0 = ppu->vram[(vram_offset + sy) & 0x7FFF];
    uint16 w1 = ppu->vram[(vram_offset + sy + 8) & 0x7FFF];
    uint8 bp0 = w0 & 0xFF;
    uint8 bp1 = (w0 >> 8) & 0xFF;
    uint8 bp2 = w1 & 0xFF;
    uint8 bp3 = (w1 >> 8) & 0xFF;

    for (int px = 0; px < 8; px++) {
      int sx = hflip ? (7 - px) : px;
      int bit = 7 - sx;
      int pixel = ((bp0 >> bit) & 1) |
                  (((bp1 >> bit) & 1) << 1) |
                  (((bp2 >> bit) & 1) << 2) |
                  (((bp3 >> bit) & 1) << 3);

      uint32 argb;
      if (pixel == 0) {
        argb = 0xFF202020; // transparent → dark bg
      } else {
        uint16 snes_color = ppu->cgram[palette * 16 + pixel];
        int r = (snes_color & 0x1F) << 3;
        int g = ((snes_color >> 5) & 0x1F) << 3;
        int b = ((snes_color >> 10) & 0x1F) << 3;
        argb = 0xFF000000 | (r << 16) | (g << 8) | b;
      }
      out[py * out_stride + px] = argb;
    }
  }
}

static void RenderBlock16x16(uint32 *out, int out_stride, int block_index) {
  Ppu *ppu = g_ed_snes->ppu;
  TileTable *tt = &tile_table.tables[block_index];

  // Top-left 8x8
  RenderOneTile8x8(out, out_stride, tt->top_left, ppu);
  // Top-right 8x8
  RenderOneTile8x8(out + 8, out_stride, tt->top_right, ppu);
  // Bottom-left 8x8
  RenderOneTile8x8(out + 8 * out_stride, out_stride, tt->bottom_left, ppu);
  // Bottom-right 8x8
  RenderOneTile8x8(out + 8 * out_stride + 8, out_stride, tt->bottom_right, ppu);
}

static void RebuildTileSheet(void) {
  for (int i = 0; i < kMaxBlocks; i++) {
    int col = i % kTileSheetCols;
    int row = i / kTileSheetCols;
    uint32 *dst = g_tilesheet + (row * 16) * (kTileSheetCols * 16) + col * 16;
    RenderBlock16x16(dst, kTileSheetCols * 16, i);
  }
}

// ---------------------------------------------------------------------------
// Load tileset from ROM into PPU buffers
// ---------------------------------------------------------------------------
#define kStateHeaderTileSets ((uint16*)RomFixedPtr(0x8fe7a7))

static void LoadTilesetDirect(int tileset_idx) {
  int gs = kTilesets[tileset_idx].graphics_set;
  TileSet *ts = get_TileSet(kStateHeaderTileSets[gs]);

  // Decompress CRE tile graphics → VRAM at byte addr 0x5000 (word addr 0x2800)
  WriteRegWord(VMAIN, 0x80);
  WriteRegWord(VMADDL, 0x2800);
  DecompressToVRAM(0xb98000, 0x5000);

  // Decompress tileset tile graphics → VRAM at addr 0
  WriteRegWord(VMADDL, 0);
  DecompressToVRAM(Load24(&ts->tiles_ptr), 0);

  // Decompress palette → g_ram+0xC200, then copy to PPU CGRAM
  DecompressToMem(Load24(&ts->palette_ptr), &g_ram[0xC200]);
  memcpy(g_ed_snes->ppu->cgram, (uint16 *)&g_ram[0xC200], 512);

  // Decompress tile tables → g_ram+0xA000 (CRE at 0xA000, tileset at 0xA800)
  DecompressToMem(0xb9a09d, g_ram + 0xA000);
  DecompressToMem(Load24(&ts->tile_table_ptr), g_ram + 0xA800);

  // Set BG1 tile base address to 0 (standard SM value)
  g_ed_snes->ppu->bgLayer[0].tileAdr = 0;
}

// ---------------------------------------------------------------------------
// Editor state
// ---------------------------------------------------------------------------
enum {
  kCanvasW = 800,
  kCanvasH = 600,
  kPaletteW = 544,
  kPaletteH = 600,
  kBlockScale = 2,
  kScaledBlock = 16 * kBlockScale,  // 32 pixels
  kPalBlockSize = 32,
  kPalCols = 16,
  kStatusBarH = 24,
  kPalHeaderH = 24,
  kPalTypeBarH = 24,
};

static SDL_Window *g_canvas_win, *g_palette_win;
static SDL_Renderer *g_canvas_rend, *g_palette_rend;
static SDL_Texture *g_canvas_tex, *g_palette_tex;
static uint32 g_canvas_wid, g_palette_wid;

static CustomRoom g_room;
static int g_selected_tile;  // tile table index (0-1023)
static int g_selected_type;  // kBlockTypes index (0=Air, 1=Solid, 2=Spike, 3=Bomb)
static int g_cam_x, g_cam_y; // camera offset in pixels (pre-scale)
static bool g_show_grid = true;
static bool g_show_collision = false;
static bool g_eraser_mode = false;
static bool g_panning = false;
static int g_pan_start_mx, g_pan_start_my;
static int g_pan_start_cx, g_pan_start_cy;
static int g_palette_scroll;  // scroll offset in rows for palette window
static bool g_playtest_pending;  // set when P is pressed, main loop returns kTileEditResult_Playtest

// Overlay state
enum {
  kOverlay_None = 0,
  kOverlay_Tileset,
  kOverlay_NewRoom,
  kOverlay_SaveName,
  kOverlay_LoadFile,
};
static int g_overlay;
static int g_overlay_sel;
static char g_text_input[256];
static int g_text_cursor;

// File list for load overlay
static char g_file_list[64][256];
static int g_file_count;

// ---------------------------------------------------------------------------
// Canvas rendering
// ---------------------------------------------------------------------------
static void BlitBlock(uint8 *pixels, int pitch, int screen_x, int screen_y,
                      int block_index, int scale) {
  int col = block_index % kTileSheetCols;
  int row = block_index / kTileSheetCols;
  uint32 *src_base = g_tilesheet + (row * 16) * (kTileSheetCols * 16) + col * 16;

  for (int py = 0; py < 16; py++) {
    for (int px = 0; px < 16; px++) {
      uint32 c = src_base[py * (kTileSheetCols * 16) + px];
      for (int sy = 0; sy < scale; sy++) {
        int dy = screen_y + py * scale + sy;
        if (dy < 0 || dy >= g_render_h) continue;
        uint32 *dst_row = (uint32 *)(pixels + dy * pitch);
        for (int sx = 0; sx < scale; sx++) {
          int dx = screen_x + px * scale + sx;
          if (dx >= 0 && dx < g_render_w)
            dst_row[dx] = c;
        }
      }
    }
  }
}

static void RenderCanvas(uint8 *pixels, int pitch, int width, int height) {
  g_render_w = width;
  g_render_h = height;

  // Clear to dark background
  for (int y = 0; y < height; y++)
    memset(pixels + y * pitch, 0x20, width * 4);
  // fix to actual dark grey
  for (int y = 0; y < height; y++) {
    uint32 *row = (uint32 *)(pixels + y * pitch);
    for (int x = 0; x < width; x++)
      row[x] = 0xFF181818;
  }

  int room_w = RoomWidthBlocks(&g_room);
  int room_h = RoomHeightBlocks(&g_room);

  // Draw room blocks
  for (int by = 0; by < room_h; by++) {
    for (int bx = 0; bx < room_w; bx++) {
      int sx = bx * kScaledBlock - g_cam_x;
      int sy = by * kScaledBlock - g_cam_y;

      // Cull off-screen blocks
      if (sx + kScaledBlock <= 0 || sx >= width) continue;
      if (sy + kScaledBlock <= 0 || sy >= height - kStatusBarH) continue;

      uint16 block = g_room.blocks[by * room_w + bx];
      int tile_idx = block & 0x3FF;
      BlitBlock(pixels, pitch, sx, sy, tile_idx, kBlockScale);

      // Collision overlay
      if (g_show_collision) {
        int btype = (block >> 12) & 0xF;
        uint32 col = 0;
        if (btype == kBlockType_Solid) col = 0x4040ff;
        else if (btype == kBlockType_Spike) col = 0xff4040;
        else if (btype == kBlockType_Bomb)  col = 0xffff40;
        else if (btype == kBlockType_Door)  col = 0x40ff40;
        else if (btype == kBlockType_Slope) col = 0xff80ff;
        if (col)
          DrawRectAlpha(pixels, pitch, sx, sy, kScaledBlock, kScaledBlock, col, 80);
      }
    }
  }

  // Grid overlay
  if (g_show_grid) {
    int room_pw = room_w * kScaledBlock;
    int room_ph = room_h * kScaledBlock;

    for (int bx = 0; bx <= room_w; bx++) {
      int sx = bx * kScaledBlock - g_cam_x;
      if (sx < 0 || sx >= width) continue;
      bool is_scroll = (bx % 16 == 0);
      uint32 col = is_scroll ? 0xFF505050 : 0xFF303030;
      int h = room_ph - g_cam_y;
      if (h > height - kStatusBarH) h = height - kStatusBarH;
      int y0 = -g_cam_y;
      if (y0 < 0) y0 = 0;
      for (int y = y0; y < h; y++) {
        uint32 *row = (uint32 *)(pixels + y * pitch);
        if (sx >= 0 && sx < width)
          row[sx] = col;
      }
    }
    for (int by = 0; by <= room_h; by++) {
      int sy = by * kScaledBlock - g_cam_y;
      if (sy < 0 || sy >= height - kStatusBarH) continue;
      bool is_scroll = (by % 16 == 0);
      uint32 col = is_scroll ? 0xFF505050 : 0xFF303030;
      uint32 *row = (uint32 *)(pixels + sy * pitch);
      int x0 = -g_cam_x;
      if (x0 < 0) x0 = 0;
      int xmax = room_pw - g_cam_x;
      if (xmax > width) xmax = width;
      for (int x = x0; x < xmax; x++)
        row[x] = col;
    }
  }

  // Samus start marker
  {
    int sx = (g_room.samus_start_x / 16) * kScaledBlock - g_cam_x;
    int sy = (g_room.samus_start_y / 16) * kScaledBlock - g_cam_y;
    DrawRectOutline(pixels, pitch, sx + 2, sy + 2, kScaledBlock - 4, kScaledBlock - 4, 0xFF00FF00);
    DrawString(pixels, pitch, sx + 4, sy + 8, "S", 0xFF00FF00);
  }

  // Cursor highlight
  {
    int mx, my;
    SDL_GetMouseState(&mx, &my);
    // Only if mouse is within the canvas area
    if (mx >= 0 && mx < width && my >= 0 && my < height - kStatusBarH) {
      int bx = (mx + g_cam_x) / kScaledBlock;
      int by = (my + g_cam_y) / kScaledBlock;
      if (bx >= 0 && bx < room_w && by >= 0 && by < room_h) {
        int sx = bx * kScaledBlock - g_cam_x;
        int sy = by * kScaledBlock - g_cam_y;
        DrawRectOutline(pixels, pitch, sx, sy, kScaledBlock, kScaledBlock, 0xFFFFFF00);
      }
    }
  }

  // Status bar
  DrawRect(pixels, pitch, 0, height - kStatusBarH, width, kStatusBarH, 0xFF101020);
  {
    char buf[256];
    const char *tool = g_eraser_mode ? "ERASER" : "PAINT";
    snprintf(buf, sizeof(buf), "%s | Tile:%d Type:%s | %dx%d | %s%s",
             tool, g_selected_tile, kBlockTypes[g_selected_type].name,
             g_room.width_scrolls, g_room.height_scrolls,
             kTilesets[g_room.tileset_index].name,
             g_room.dirty ? " *" : "");
    DrawString(pixels, pitch, 4, height - kStatusBarH + 4, buf, 0xFFCCCCCC);
    DrawString(pixels, pitch, 4, height - kStatusBarH + 14,
               "G=grid C=col E=erase T=tileset ^S=save ^O=load ^N=new P=play",
               0xFF888888);
  }

  // Tileset picker overlay
  if (g_overlay == kOverlay_Tileset) {
    int ow = 280, oh = kNumTilesets * 16 + 32;
    int ox = (width - ow) / 2, oy = (height - oh) / 2;
    DrawRect(pixels, pitch, ox, oy, ow, oh, 0xFF101830);
    DrawRectOutline(pixels, pitch, ox, oy, ow, oh, 0xFF4060A0);
    DrawString(pixels, pitch, ox + 8, oy + 8, "SELECT TILESET (Esc=cancel)", 0xFFFFFF00);
    for (int i = 0; i < kNumTilesets; i++) {
      uint32 col = (i == g_overlay_sel) ? 0xFFFFFFFF : 0xFFAAAAAA;
      if (i == g_overlay_sel)
        DrawRect(pixels, pitch, ox + 4, oy + 24 + i * 16, ow - 8, 14, 0xFF203060);
      DrawString(pixels, pitch, ox + 8, oy + 24 + i * 16 + 3, kTilesets[i].name, col);
    }
  }

  // New room overlay
  if (g_overlay == kOverlay_NewRoom) {
    int ow = 320, oh = 80;
    int ox = (width - ow) / 2, oy = (height - oh) / 2;
    DrawRect(pixels, pitch, ox, oy, ow, oh, 0xFF101830);
    DrawRectOutline(pixels, pitch, ox, oy, ow, oh, 0xFF4060A0);
    DrawString(pixels, pitch, ox + 8, oy + 8, "NEW ROOM SIZE (Enter=create, Esc=cancel)", 0xFFFFFF00);
    char buf[64];
    int new_ws = (g_overlay_sel >> 4) + 1;
    int new_hs = (g_overlay_sel & 0xF) + 1;
    snprintf(buf, sizeof(buf), "Width: %d screens  (Left/Right)", new_ws);
    DrawString(pixels, pitch, ox + 8, oy + 32, buf, 0xFFFFFFFF);
    snprintf(buf, sizeof(buf), "Height: %d screens  (Up/Down)", new_hs);
    DrawString(pixels, pitch, ox + 8, oy + 48, buf, 0xFFFFFFFF);
  }

  // Save name overlay
  if (g_overlay == kOverlay_SaveName) {
    int ow = 400, oh = 64;
    int ox = (width - ow) / 2, oy = (height - oh) / 2;
    DrawRect(pixels, pitch, ox, oy, ow, oh, 0xFF101830);
    DrawRectOutline(pixels, pitch, ox, oy, ow, oh, 0xFF4060A0);
    DrawString(pixels, pitch, ox + 8, oy + 8, "SAVE AS (Enter=save, Esc=cancel)", 0xFFFFFF00);
    DrawString(pixels, pitch, ox + 8, oy + 32, g_text_input, 0xFFFFFFFF);
    // cursor blink
    if ((SDL_GetTicks() / 500) & 1)
      DrawRect(pixels, pitch, ox + 8 + g_text_cursor * 8, oy + 32, 2, 10, 0xFFFFFFFF);
  }

  // Load file overlay
  if (g_overlay == kOverlay_LoadFile) {
    int ow = 400, oh = g_file_count * 16 + 48;
    if (oh > height - 40) oh = height - 40;
    int ox = (width - ow) / 2, oy = (height - oh) / 2;
    DrawRect(pixels, pitch, ox, oy, ow, oh, 0xFF101830);
    DrawRectOutline(pixels, pitch, ox, oy, ow, oh, 0xFF4060A0);
    DrawString(pixels, pitch, ox + 8, oy + 8, "LOAD ROOM (Enter=load, Esc=cancel)", 0xFFFFFF00);
    if (g_file_count == 0) {
      DrawString(pixels, pitch, ox + 8, oy + 32, "No files in custom_rooms/", 0xFFAAAAAA);
    } else {
      for (int i = 0; i < g_file_count; i++) {
        int fy = oy + 28 + i * 16;
        if (fy + 16 > oy + oh) break;
        uint32 col = (i == g_overlay_sel) ? 0xFFFFFFFF : 0xFFAAAAAA;
        if (i == g_overlay_sel)
          DrawRect(pixels, pitch, ox + 4, fy, ow - 8, 14, 0xFF203060);
        DrawString(pixels, pitch, ox + 8, fy + 3, g_file_list[i], col);
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Palette window rendering
// ---------------------------------------------------------------------------
static void RenderPalette(uint8 *pixels, int pitch, int width, int height) {
  g_render_w = width;
  g_render_h = height;

  // Clear
  for (int y = 0; y < height; y++) {
    uint32 *row = (uint32 *)(pixels + y * pitch);
    for (int x = 0; x < width; x++)
      row[x] = 0xFF1A1A2A;
  }

  // Header
  DrawString(pixels, pitch, 4, 4, "TILE PALETTE", 0xFFFFFF00);
  {
    char buf[64];
    snprintf(buf, sizeof(buf), "Selected: %d", g_selected_tile);
    DrawString(pixels, pitch, 4, 14, buf, 0xFFAAAAAA);
  }

  // Tile grid
  int grid_y0 = kPalHeaderH;
  int grid_h = height - grid_y0 - kPalTypeBarH;
  int visible_rows = grid_h / kPalBlockSize;
  int total_rows = (kMaxBlocks + kPalCols - 1) / kPalCols;

  for (int vy = 0; vy < visible_rows + 1; vy++) {
    int row = vy + g_palette_scroll;
    if (row >= total_rows) break;
    for (int col = 0; col < kPalCols; col++) {
      int idx = row * kPalCols + col;
      if (idx >= kMaxBlocks) break;

      int dx = col * kPalBlockSize;
      int dy = grid_y0 + vy * kPalBlockSize;
      if (dy + kPalBlockSize > height - kPalTypeBarH) continue;

      // Render the block from tilesheet (scale=2)
      int ts_col = idx % kTileSheetCols;
      int ts_row = idx / kTileSheetCols;
      uint32 *src_base = g_tilesheet + (ts_row * 16) * (kTileSheetCols * 16) + ts_col * 16;

      for (int py = 0; py < 16; py++) {
        for (int px = 0; px < 16; px++) {
          uint32 c = src_base[py * (kTileSheetCols * 16) + px];
          int sy = dy + py * 2;
          int sx = dx + px * 2;
          if (sy >= 0 && sy + 1 < height && sx >= 0 && sx + 1 < width) {
            uint32 *r0 = (uint32 *)(pixels + sy * pitch);
            uint32 *r1 = (uint32 *)(pixels + (sy + 1) * pitch);
            r0[sx] = r0[sx + 1] = c;
            r1[sx] = r1[sx + 1] = c;
          }
        }
      }

      // Highlight selected
      if (idx == g_selected_tile)
        DrawRectOutline(pixels, pitch, dx, dy, kPalBlockSize, kPalBlockSize, 0xFFFFFF00);
    }
  }

  // Block type selector bar at bottom
  int bar_y = height - kPalTypeBarH;
  DrawRect(pixels, pitch, 0, bar_y, width, kPalTypeBarH, 0xFF101020);
  DrawString(pixels, pitch, 4, bar_y + 2, "Type:", 0xFFCCCCCC);
  for (int i = 0; i < kNumBlockTypes; i++) {
    int bx = 50 + i * 80;
    uint32 bg = (i == g_selected_type) ? 0xFF304080 : 0xFF202040;
    DrawRect(pixels, pitch, bx, bar_y + 1, 72, kPalTypeBarH - 2, bg);
    DrawRectOutline(pixels, pitch, bx, bar_y + 1, 72, kPalTypeBarH - 2,
                    i == g_selected_type ? 0xFFFFFF00 : 0xFF606060);
    char lbl[16];
    snprintf(lbl, sizeof(lbl), "%d:%s", i + 1, kBlockTypes[i].name);
    DrawString(pixels, pitch, bx + 4, bar_y + 8, lbl, 0xFFFFFFFF);
  }
}

// ---------------------------------------------------------------------------
// File list scanning for load overlay
// ---------------------------------------------------------------------------
static void ScanFileList(void) {
  g_file_count = 0;
#ifdef _WIN32
  WIN32_FIND_DATA fd;
  HANDLE h = FindFirstFile("custom_rooms\\*.json", &fd);
  if (h == INVALID_HANDLE_VALUE) return;
  do {
    if (g_file_count < 64)
      snprintf(g_file_list[g_file_count++], 256, "%s", fd.cFileName);
  } while (FindNextFile(h, &fd));
  FindClose(h);
#else
  DIR *d = opendir("custom_rooms");
  if (!d) return;
  struct dirent *ent;
  while ((ent = readdir(d)) != NULL) {
    size_t len = strlen(ent->d_name);
    if (len > 5 && strcmp(ent->d_name + len - 5, ".json") == 0) {
      if (g_file_count < 64)
        snprintf(g_file_list[g_file_count++], 256, "%s", ent->d_name);
    }
  }
  closedir(d);
#endif
}

// ---------------------------------------------------------------------------
// Place a block at grid position
// ---------------------------------------------------------------------------
static void PlaceBlock(int bx, int by) {
  int room_w = RoomWidthBlocks(&g_room);
  int room_h = RoomHeightBlocks(&g_room);
  if (bx < 0 || bx >= room_w || by < 0 || by >= room_h) return;

  int idx = by * room_w + bx;
  if (g_eraser_mode) {
    g_room.blocks[idx] = 0;
    g_room.bts[idx] = 0;
  } else {
    uint16 block = (uint16)((kBlockTypes[g_selected_type].type << 12) | (g_selected_tile & 0x3FF));
    g_room.blocks[idx] = block;
    g_room.bts[idx] = 0;
  }
  g_room.dirty = true;
}

// ---------------------------------------------------------------------------
// Event handlers
// ---------------------------------------------------------------------------
static bool g_mouse_painting;

static void HandleCanvasMouse(SDL_Event *ev) {
  int mx = ev->button.x;
  int my = ev->button.y;

  if (ev->type == SDL_MOUSEBUTTONDOWN) {
    if (ev->button.button == SDL_BUTTON_LEFT && g_overlay == kOverlay_None) {
      g_mouse_painting = true;
      int bx = (mx + g_cam_x) / kScaledBlock;
      int by = (my + g_cam_y) / kScaledBlock;
      PlaceBlock(bx, by);
    } else if (ev->button.button == SDL_BUTTON_RIGHT) {
      g_panning = true;
      g_pan_start_mx = mx;
      g_pan_start_my = my;
      g_pan_start_cx = g_cam_x;
      g_pan_start_cy = g_cam_y;
    } else if (ev->button.button == SDL_BUTTON_MIDDLE && g_overlay == kOverlay_None) {
      // Eyedropper
      int bx = (mx + g_cam_x) / kScaledBlock;
      int by = (my + g_cam_y) / kScaledBlock;
      int room_w = RoomWidthBlocks(&g_room);
      int room_h = RoomHeightBlocks(&g_room);
      if (bx >= 0 && bx < room_w && by >= 0 && by < room_h) {
        uint16 block = g_room.blocks[by * room_w + bx];
        g_selected_tile = block & 0x3FF;
        int bt = (block >> 12) & 0xF;
        for (int i = 0; i < kNumBlockTypes; i++) {
          if (kBlockTypes[i].type == bt) { g_selected_type = i; break; }
        }
        g_eraser_mode = false;
      }
    }
  } else if (ev->type == SDL_MOUSEBUTTONUP) {
    if (ev->button.button == SDL_BUTTON_LEFT)
      g_mouse_painting = false;
    if (ev->button.button == SDL_BUTTON_RIGHT)
      g_panning = false;
  }
}

static void HandleCanvasMotion(SDL_Event *ev) {
  int mx = ev->motion.x, my = ev->motion.y;
  if (g_panning) {
    g_cam_x = g_pan_start_cx - (mx - g_pan_start_mx);
    g_cam_y = g_pan_start_cy - (my - g_pan_start_my);
  } else if (g_mouse_painting && g_overlay == kOverlay_None) {
    int bx = (mx + g_cam_x) / kScaledBlock;
    int by = (my + g_cam_y) / kScaledBlock;
    PlaceBlock(bx, by);
  }
}

static void HandlePaletteMouse(SDL_Event *ev) {
  if (ev->type != SDL_MOUSEBUTTONDOWN || ev->button.button != SDL_BUTTON_LEFT)
    return;
  int mx = ev->button.x, my = ev->button.y;

  // Check type bar
  int bar_y = kPaletteH - kPalTypeBarH;
  if (my >= bar_y) {
    for (int i = 0; i < kNumBlockTypes; i++) {
      int bx = 50 + i * 80;
      if (mx >= bx && mx < bx + 72) {
        g_selected_type = i;
        return;
      }
    }
    return;
  }

  // Check tile grid
  if (my >= kPalHeaderH) {
    int col = mx / kPalBlockSize;
    int row = (my - kPalHeaderH) / kPalBlockSize + g_palette_scroll;
    int idx = row * kPalCols + col;
    if (idx >= 0 && idx < kMaxBlocks) {
      g_selected_tile = idx;
      g_eraser_mode = false;
    }
  }
}

static void HandlePaletteScroll(SDL_Event *ev) {
  int total_rows = (kMaxBlocks + kPalCols - 1) / kPalCols;
  g_palette_scroll -= ev->wheel.y * 3;
  if (g_palette_scroll < 0) g_palette_scroll = 0;
  if (g_palette_scroll > total_rows - 5) g_palette_scroll = total_rows - 5;
}

static void DoSave(const char *name) {
  char path[512];
#ifdef _WIN32
  _mkdir("custom_rooms");
#else
  mkdir("custom_rooms", 0755);
#endif
  snprintf(path, sizeof(path), "custom_rooms/%s.json", name);
  snprintf(g_room.name, sizeof(g_room.name), "%s", name);
  snprintf(g_room.filename, sizeof(g_room.filename), "%s", path);
  if (TileEditJson_Save(&g_room, path)) {
    g_room.dirty = false;
    printf("Saved: %s\n", path);
  } else {
    fprintf(stderr, "Failed to save: %s\n", path);
  }
}

static void DoLoad(const char *filename) {
  char path[512];
  snprintf(path, sizeof(path), "custom_rooms/%s", filename);
  CustomRoom tmp;
  memset(&tmp, 0, sizeof(tmp));
  if (TileEditJson_Load(&tmp, path)) {
    CustomRoom_Free(&g_room);
    g_room = tmp;
    snprintf(g_room.filename, sizeof(g_room.filename), "%s", path);
    g_room.dirty = false;
    // Reload tileset if changed
    LoadTilesetDirect(g_room.tileset_index);
    RebuildTileSheet();
    g_cam_x = g_cam_y = 0;
    printf("Loaded: %s\n", path);
  } else {
    fprintf(stderr, "Failed to load: %s\n", path);
    if (tmp.blocks) free(tmp.blocks);
    if (tmp.bts) free(tmp.bts);
  }
}

static void HandleOverlayKey(SDL_Keycode key) {
  switch (g_overlay) {
  case kOverlay_Tileset:
    if (key == SDLK_ESCAPE) { g_overlay = kOverlay_None; return; }
    if (key == SDLK_UP && g_overlay_sel > 0) g_overlay_sel--;
    if (key == SDLK_DOWN && g_overlay_sel < kNumTilesets - 1) g_overlay_sel++;
    if (key == SDLK_RETURN) {
      g_room.tileset_index = g_overlay_sel;
      LoadTilesetDirect(g_overlay_sel);
      RebuildTileSheet();
      g_room.dirty = true;
      g_overlay = kOverlay_None;
    }
    break;

  case kOverlay_NewRoom:
    if (key == SDLK_ESCAPE) { g_overlay = kOverlay_None; return; }
    {
      int ws = (g_overlay_sel >> 4) + 1;
      int hs = (g_overlay_sel & 0xF) + 1;
      if (key == SDLK_RIGHT && ws < 16) g_overlay_sel += 0x10;
      if (key == SDLK_LEFT && ws > 1)   g_overlay_sel -= 0x10;
      if (key == SDLK_DOWN && hs < 16)  g_overlay_sel += 1;
      if (key == SDLK_UP && hs > 1)     g_overlay_sel -= 1;
      if (key == SDLK_RETURN) {
        ws = (g_overlay_sel >> 4) + 1;
        hs = (g_overlay_sel & 0xF) + 1;
        CustomRoom_Free(&g_room);
        int old_ts = g_room.tileset_index;
        CustomRoom_Init(&g_room, ws, hs);
        g_room.tileset_index = old_ts;
        g_cam_x = g_cam_y = 0;
        g_overlay = kOverlay_None;
      }
    }
    break;

  case kOverlay_SaveName:
    if (key == SDLK_ESCAPE) { g_overlay = kOverlay_None; SDL_StopTextInput(); return; }
    if (key == SDLK_RETURN) {
      SDL_StopTextInput();
      if (g_text_cursor > 0) {
        DoSave(g_text_input);
      }
      g_overlay = kOverlay_None;
      return;
    }
    if (key == SDLK_BACKSPACE && g_text_cursor > 0) {
      g_text_input[--g_text_cursor] = 0;
    }
    break;

  case kOverlay_LoadFile:
    if (key == SDLK_ESCAPE) { g_overlay = kOverlay_None; return; }
    if (key == SDLK_UP && g_overlay_sel > 0) g_overlay_sel--;
    if (key == SDLK_DOWN && g_overlay_sel < g_file_count - 1) g_overlay_sel++;
    if (key == SDLK_RETURN && g_file_count > 0) {
      DoLoad(g_file_list[g_overlay_sel]);
      g_overlay = kOverlay_None;
    }
    break;
  }
}

static void HandleCanvasKey(SDL_Keycode key, uint16 mod) {
  // Overlays capture all input
  if (g_overlay != kOverlay_None) {
    HandleOverlayKey(key);
    return;
  }

  bool ctrl = (mod & KMOD_CTRL) != 0;

  if (ctrl) {
    if (key == SDLK_s) {
      if (g_room.filename[0]) {
        // Re-save to same file
        if (TileEditJson_Save(&g_room, g_room.filename)) {
          g_room.dirty = false;
          printf("Saved: %s\n", g_room.filename);
        }
      } else {
        g_overlay = kOverlay_SaveName;
        memset(g_text_input, 0, sizeof(g_text_input));
        g_text_cursor = 0;
        snprintf(g_text_input, sizeof(g_text_input), "%s", g_room.name);
        g_text_cursor = (int)strlen(g_text_input);
        SDL_StartTextInput();
      }
      return;
    }
    if (key == SDLK_o) {
      ScanFileList();
      g_overlay = kOverlay_LoadFile;
      g_overlay_sel = 0;
      return;
    }
    if (key == SDLK_n) {
      g_overlay = kOverlay_NewRoom;
      g_overlay_sel = 0;  // 1x1
      return;
    }
    return;
  }

  switch (key) {
  case SDLK_ESCAPE:
    // handled in main loop
    break;
  case SDLK_g: g_show_grid = !g_show_grid; break;
  case SDLK_c: g_show_collision = !g_show_collision; break;
  case SDLK_e: g_eraser_mode = !g_eraser_mode; break;
  case SDLK_t:
    g_overlay = kOverlay_Tileset;
    g_overlay_sel = g_room.tileset_index;
    break;
  case SDLK_1: g_selected_type = 0; break;
  case SDLK_2: g_selected_type = 1; break;
  case SDLK_3: g_selected_type = 2; break;
  case SDLK_4: g_selected_type = 3; break;
  case SDLK_UP:    g_cam_y -= kScaledBlock * 4; break;
  case SDLK_DOWN:  g_cam_y += kScaledBlock * 4; break;
  case SDLK_LEFT:  g_cam_x -= kScaledBlock * 4; break;
  case SDLK_RIGHT: g_cam_x += kScaledBlock * 4; break;
  case SDLK_s:
    // Set Samus start position at cursor
    {
      int mx, my;
      SDL_GetMouseState(&mx, &my);
      int bx = (mx + g_cam_x) / kScaledBlock;
      int by = (my + g_cam_y) / kScaledBlock;
      if (bx >= 0 && bx < RoomWidthBlocks(&g_room) &&
          by >= 0 && by < RoomHeightBlocks(&g_room)) {
        g_room.samus_start_x = bx * 16 + 8;
        g_room.samus_start_y = by * 16 + 8;
        g_room.dirty = true;
      }
    }
    break;
  case SDLK_p:
    // Playtest: signal main loop to switch to game mode
    g_playtest_pending = true;
    break;
  }
}

// ---------------------------------------------------------------------------
// Playtest: inject custom room data into game state
// ---------------------------------------------------------------------------
static bool g_playtest_injected;

bool TileEdit_HasPendingPlaytest(void) {
  return g_playtest_pending && !g_playtest_injected;
}

void TileEdit_InjectCustomRoom(void) {
  int w = RoomWidthBlocks(&g_room);
  int h = RoomHeightBlocks(&g_room);
  int total = w * h;

  // Set room dimensions
  room_width_in_blocks = w;
  room_height_in_blocks = h;
  room_width_in_scrolls = g_room.width_scrolls;
  room_height_in_scrolls = g_room.height_scrolls;
  room_size_in_blocks = total;

  // Copy block data
  for (int i = 0; i < total && i < 0x5000 / 2; i++)
    level_data[i] = g_room.blocks[i];

  // Copy BTS data
  for (int i = 0; i < total && i < 0x5000; i++)
    BTS[i] = g_room.bts[i];

  // Set all screens visible
  int num_scrolls = g_room.width_scrolls * g_room.height_scrolls;
  for (int i = 0; i < num_scrolls && i < 50; i++)
    scrolls[i] = 2;  // 2 = visible

  // Set Samus position
  samus_x_pos = g_room.samus_start_x;
  samus_y_pos = g_room.samus_start_y;

  RtlSyncAll();
  g_playtest_injected = true;
}

void TileEdit_GetPlaytestWarp(uint8 *area, uint8 *station) {
  int ti = g_room.tileset_index;
  if (ti < 0 || ti >= kNumTilesets) ti = 0;
  *area = kTilesetToArea[ti];
  *station = kTilesetToStation[ti];
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
int TileEdit_MainLoop(Snes *snes) {
  g_ed_snes = snes;

  // Create canvas window
  g_canvas_win = SDL_CreateWindow("SM Tile Editor",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    kCanvasW, kCanvasH, SDL_WINDOW_RESIZABLE);
  g_canvas_wid = SDL_GetWindowID(g_canvas_win);
  g_canvas_rend = SDL_CreateRenderer(g_canvas_win, -1, SDL_RENDERER_SOFTWARE);
  g_canvas_tex = SDL_CreateTexture(g_canvas_rend, SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING, kCanvasW, kCanvasH);

  // Create palette window
  g_palette_win = SDL_CreateWindow("Tile Palette",
    SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
    kPaletteW, kPaletteH, SDL_WINDOW_RESIZABLE);
  g_palette_wid = SDL_GetWindowID(g_palette_win);
  g_palette_rend = SDL_CreateRenderer(g_palette_win, -1, SDL_RENDERER_SOFTWARE);
  g_palette_tex = SDL_CreateTexture(g_palette_rend, SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING, kPaletteW, kPaletteH);

  // Load default tileset (Crateria Surface)
  LoadTilesetDirect(0);
  RebuildTileSheet();

  // Create default 1x1 room
  CustomRoom_Init(&g_room, 1, 1);

  bool running = true;
  int canvas_w = kCanvasW, canvas_h = kCanvasH;
  int palette_w = kPaletteW, palette_h = kPaletteH;

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        running = false;
        break;

      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_CLOSE) {
          if (event.window.windowID == g_canvas_wid)
            running = false;
          else if (event.window.windowID == g_palette_wid)
            running = false;
        }
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
          if (event.window.windowID == g_canvas_wid) {
            canvas_w = event.window.data1;
            canvas_h = event.window.data2;
            SDL_DestroyTexture(g_canvas_tex);
            g_canvas_tex = SDL_CreateTexture(g_canvas_rend, SDL_PIXELFORMAT_ARGB8888,
              SDL_TEXTUREACCESS_STREAMING, canvas_w, canvas_h);
          } else if (event.window.windowID == g_palette_wid) {
            palette_w = event.window.data1;
            palette_h = event.window.data2;
            SDL_DestroyTexture(g_palette_tex);
            g_palette_tex = SDL_CreateTexture(g_palette_rend, SDL_PIXELFORMAT_ARGB8888,
              SDL_TEXTUREACCESS_STREAMING, palette_w, palette_h);
          }
        }
        break;

      case SDL_KEYDOWN:
        if (event.key.keysym.sym == SDLK_ESCAPE && g_overlay == kOverlay_None) {
          running = false;
          break;
        }
        if (event.key.windowID == g_canvas_wid)
          HandleCanvasKey(event.key.keysym.sym, event.key.keysym.mod);
        break;

      case SDL_TEXTINPUT:
        if (g_overlay == kOverlay_SaveName) {
          int len = (int)strlen(event.text.text);
          for (int i = 0; i < len && g_text_cursor < 250; i++) {
            char c = event.text.text[i];
            if (c >= 32 && c != '/' && c != '\\') {
              g_text_input[g_text_cursor++] = c;
              g_text_input[g_text_cursor] = 0;
            }
          }
        }
        break;

      case SDL_MOUSEBUTTONDOWN:
      case SDL_MOUSEBUTTONUP:
        if (event.button.windowID == g_canvas_wid)
          HandleCanvasMouse(&event);
        else if (event.button.windowID == g_palette_wid)
          HandlePaletteMouse(&event);
        break;

      case SDL_MOUSEMOTION:
        if (event.motion.windowID == g_canvas_wid)
          HandleCanvasMotion(&event);
        break;

      case SDL_MOUSEWHEEL:
        if (event.wheel.windowID == g_palette_wid)
          HandlePaletteScroll(&event);
        else if (event.wheel.windowID == g_canvas_wid) {
          g_cam_y -= event.wheel.y * kScaledBlock * 3;
        }
        break;
      }
    }

    // Render canvas
    {
      uint8 *pixels;
      int pitch;
      SDL_LockTexture(g_canvas_tex, NULL, (void **)&pixels, &pitch);
      RenderCanvas(pixels, pitch, canvas_w, canvas_h);
      SDL_UnlockTexture(g_canvas_tex);
      SDL_RenderClear(g_canvas_rend);
      SDL_RenderCopy(g_canvas_rend, g_canvas_tex, NULL, NULL);
      SDL_RenderPresent(g_canvas_rend);
    }

    // Render palette
    {
      uint8 *pixels;
      int pitch;
      SDL_LockTexture(g_palette_tex, NULL, (void **)&pixels, &pitch);
      RenderPalette(pixels, pitch, palette_w, palette_h);
      SDL_UnlockTexture(g_palette_tex);
      SDL_RenderClear(g_palette_rend);
      SDL_RenderCopy(g_palette_rend, g_palette_tex, NULL, NULL);
      SDL_RenderPresent(g_palette_rend);
    }

    // Check if playtest was requested
    if (g_playtest_pending) {
      running = false;
    }

    SDL_Delay(16);
  }

  int result = g_playtest_pending ? kTileEditResult_Playtest : kTileEditResult_Quit;

  // Cleanup windows (room data preserved for playtest)
  SDL_DestroyTexture(g_canvas_tex);
  SDL_DestroyTexture(g_palette_tex);
  SDL_DestroyRenderer(g_canvas_rend);
  SDL_DestroyRenderer(g_palette_rend);
  SDL_DestroyWindow(g_canvas_win);
  SDL_DestroyWindow(g_palette_win);
  g_canvas_win = g_palette_win = NULL;

  if (result == kTileEditResult_Quit)
    CustomRoom_Free(&g_room);

  g_playtest_pending = false;
  g_playtest_injected = false;
  return result;
}
