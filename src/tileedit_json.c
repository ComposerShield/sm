#include "tileedit_json.h"
#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Forward declare CustomRoom structure (matches tileedit.c)
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

// ---------------------------------------------------------------------------
// JSON emitter (fprintf-based)
// ---------------------------------------------------------------------------
bool TileEditJson_Save(const CustomRoom *room, const char *path) {
  FILE *f = fopen(path, "w");
  if (!f) return false;

  int w = room->width_scrolls * 16;
  int h = room->height_scrolls * 16;
  int total = w * h;

  fprintf(f, "{\n");
  fprintf(f, "  \"name\": \"%s\",\n", room->name);
  fprintf(f, "  \"version\": 1,\n");
  fprintf(f, "  \"width_scrolls\": %d,\n", room->width_scrolls);
  fprintf(f, "  \"height_scrolls\": %d,\n", room->height_scrolls);
  fprintf(f, "  \"tileset_index\": %d,\n", room->tileset_index);
  fprintf(f, "  \"samus_start_x\": %d,\n", room->samus_start_x);
  fprintf(f, "  \"samus_start_y\": %d,\n", room->samus_start_y);

  // Blocks as hex strings
  fprintf(f, "  \"blocks\": [");
  for (int i = 0; i < total; i++) {
    if (i > 0) fprintf(f, ",");
    if (i % 16 == 0) fprintf(f, "\n    ");
    fprintf(f, "\"%04X\"", room->blocks[i]);
  }
  fprintf(f, "\n  ],\n");

  // BTS as hex strings
  fprintf(f, "  \"bts\": [");
  for (int i = 0; i < total; i++) {
    if (i > 0) fprintf(f, ",");
    if (i % 16 == 0) fprintf(f, "\n    ");
    fprintf(f, "\"%02X\"", room->bts[i]);
  }
  fprintf(f, "\n  ]\n");

  fprintf(f, "}\n");
  fclose(f);
  return true;
}

// ---------------------------------------------------------------------------
// Minimal JSON parser (recursive descent)
// ---------------------------------------------------------------------------
typedef struct JsonParser {
  const char *p;
  const char *end;
} JsonParser;

static void SkipWs(JsonParser *jp) {
  while (jp->p < jp->end && (*jp->p == ' ' || *jp->p == '\t' || *jp->p == '\n' || *jp->p == '\r'))
    jp->p++;
}

static bool Expect(JsonParser *jp, char c) {
  SkipWs(jp);
  if (jp->p < jp->end && *jp->p == c) { jp->p++; return true; }
  return false;
}

static bool ParseString(JsonParser *jp, char *out, int max_len) {
  SkipWs(jp);
  if (jp->p >= jp->end || *jp->p != '"') return false;
  jp->p++;
  int i = 0;
  while (jp->p < jp->end && *jp->p != '"') {
    if (*jp->p == '\\') { jp->p++; if (jp->p >= jp->end) return false; }
    if (i < max_len - 1) out[i++] = *jp->p;
    jp->p++;
  }
  out[i] = 0;
  if (jp->p < jp->end) jp->p++; // skip closing quote
  return true;
}

static bool ParseInt(JsonParser *jp, int *out) {
  SkipWs(jp);
  char *end;
  long v = strtol(jp->p, &end, 10);
  if (end == jp->p) return false;
  *out = (int)v;
  jp->p = end;
  return true;
}

static bool ParseHexString(JsonParser *jp, uint32 *out) {
  char buf[16];
  if (!ParseString(jp, buf, sizeof(buf))) return false;
  *out = (uint32)strtol(buf, NULL, 16);
  return true;
}

bool TileEditJson_Load(CustomRoom *room, const char *path) {
  FILE *f = fopen(path, "rb");
  if (!f) return false;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *data = (char *)malloc(size + 1);
  if (!data) { fclose(f); return false; }
  fread(data, 1, size, f);
  data[size] = 0;
  fclose(f);

  JsonParser jp = { data, data + size };
  memset(room, 0, sizeof(*room));
  room->width_scrolls = 1;
  room->height_scrolls = 1;

  if (!Expect(&jp, '{')) { free(data); return false; }

  while (jp.p < jp.end) {
    SkipWs(&jp);
    if (*jp.p == '}') { jp.p++; break; }
    if (*jp.p == ',') { jp.p++; continue; }

    char key[64];
    if (!ParseString(&jp, key, sizeof(key))) break;
    if (!Expect(&jp, ':')) break;

    if (strcmp(key, "name") == 0) {
      ParseString(&jp, room->name, sizeof(room->name));
    } else if (strcmp(key, "version") == 0) {
      int v; ParseInt(&jp, &v);
    } else if (strcmp(key, "width_scrolls") == 0) {
      ParseInt(&jp, &room->width_scrolls);
    } else if (strcmp(key, "height_scrolls") == 0) {
      ParseInt(&jp, &room->height_scrolls);
    } else if (strcmp(key, "tileset_index") == 0) {
      ParseInt(&jp, &room->tileset_index);
    } else if (strcmp(key, "samus_start_x") == 0) {
      int v; ParseInt(&jp, &v); room->samus_start_x = v;
    } else if (strcmp(key, "samus_start_y") == 0) {
      int v; ParseInt(&jp, &v); room->samus_start_y = v;
    } else if (strcmp(key, "blocks") == 0) {
      int total = room->width_scrolls * 16 * room->height_scrolls * 16;
      room->blocks = (uint16 *)calloc(total, sizeof(uint16));
      if (!Expect(&jp, '[')) break;
      for (int i = 0; i < total; i++) {
        if (i > 0) { SkipWs(&jp); if (*jp.p == ',') jp.p++; }
        uint32 v;
        if (!ParseHexString(&jp, &v)) break;
        room->blocks[i] = (uint16)v;
      }
      SkipWs(&jp);
      if (*jp.p == ']') jp.p++;
    } else if (strcmp(key, "bts") == 0) {
      int total = room->width_scrolls * 16 * room->height_scrolls * 16;
      room->bts = (uint8 *)calloc(total, 1);
      if (!Expect(&jp, '[')) break;
      for (int i = 0; i < total; i++) {
        if (i > 0) { SkipWs(&jp); if (*jp.p == ',') jp.p++; }
        uint32 v;
        if (!ParseHexString(&jp, &v)) break;
        room->bts[i] = (uint8)v;
      }
      SkipWs(&jp);
      if (*jp.p == ']') jp.p++;
    } else {
      // Skip unknown value
      SkipWs(&jp);
      if (*jp.p == '"') {
        char tmp[256]; ParseString(&jp, tmp, sizeof(tmp));
      } else if (*jp.p == '[') {
        int depth = 1; jp.p++;
        while (jp.p < jp.end && depth > 0) {
          if (*jp.p == '[') depth++;
          else if (*jp.p == ']') depth--;
          jp.p++;
        }
      } else {
        int v; ParseInt(&jp, &v);
      }
    }
  }

  free(data);

  // Ensure blocks/bts are allocated even if not in file
  int total = room->width_scrolls * 16 * room->height_scrolls * 16;
  if (!room->blocks) room->blocks = (uint16 *)calloc(total, sizeof(uint16));
  if (!room->bts) room->bts = (uint8 *)calloc(total, 1);

  return true;
}
