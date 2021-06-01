#include <tonc.h>

#include "Benchmark.h"
#include "Config.h"
#include "HammingWeight.h"
#include "Protocol.h"
#include "SPISlave.h"

#define DIFFS_BUFFER ((u32*)MEM_VRAM_OBJ)
#define PALETTE_BUFFER pal_obj_mem

// -----
// STATE
// -----

SPISlave* spiSlave = new SPISlave();

typedef struct {
  u32 blindFrames;
  u32 expectedPixels;
  bool isReady;
} State;

// ---------
// BENCHMARK
// ---------

#ifdef BENCHMARK
int main() {
  Benchmark::init();
  Benchmark::mainLoop();
  return 0;
}
#endif

// ------------
// DECLARATIONS
// ------------

void init();
void mainLoop();
void receiveDiffs(State& state);
void receivePalette();
void receivePixels(State& state);
void onVBlank(State& state);
bool sync(State& state, u32 local, u32 remote);
bool hasPixelChanged(u32 cursor);
u32 x(u32 cursor);
u32 y(u32 cursor);

// -----------
// DEFINITIONS
// -----------

#ifndef BENCHMARK
int main() {
  init();
  mainLoop();
  return 0;
}
#endif

inline void init() {
  REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;
}

CODE_IWRAM void mainLoop() {
  State state;
  state.blindFrames = 0;
  state.expectedPixels = 0;
  state.isReady = false;

  while (true) {
    if (state.isReady) {
      if (IS_VBLANK)
        onVBlank(state);
      else
        continue;
    }

    state.blindFrames = 0;
    state.expectedPixels = 0;
    spiSlave->transfer(CMD_RESET);

    if (!sync(state, CMD_FRAME_START_GBA, CMD_FRAME_START_RPI))
      continue;

    receiveDiffs(state);

    if (!sync(state, CMD_PALETTE_START_GBA, CMD_PALETTE_START_RPI))
      continue;

    receivePalette();

    if (!sync(state, CMD_PIXELS_START_GBA, CMD_PIXELS_START_RPI))
      continue;

    receivePixels(state);

    sync(state, CMD_FRAME_END_GBA, CMD_FRAME_END_RPI);
    state.isReady = true;
  }
}

inline void receiveDiffs(State& state) {
  for (u32 i = 0; i < TEMPORAL_DIFF_SIZE; i += COLORS_PER_PACKET) {
    u32 packet = spiSlave->transfer(0);
    DIFFS_BUFFER[i] = packet;
    state.expectedPixels += HammingWeight(packet);
  }
}

inline void receivePalette() {
  for (u32 i = 0; i < PALETTE_COLORS; i += COLORS_PER_PACKET) {
    u32 packet = spiSlave->transfer(0);
    PALETTE_BUFFER[i] = packet & 0xffff;
    PALETTE_BUFFER[i + 1] = (packet >> 16) & 0xffff;
  }
}

inline void receivePixels(State& state) {
  u32 cursor = 0;

  for (u32 i = 0; i < state.expectedPixels; i += PIXELS_PER_PACKET) {
    u32 packet = spiSlave->transfer(0);

    for (u32 byte = 0; byte < PIXELS_PER_PACKET; byte++) {
      u32 xPos = x(cursor);
      u32 yPos = y(cursor);
      if (xPos >= RENDER_WIDTH || yPos >= RENDER_HEIGHT)
        break;

      u8 color = (packet >> (byte * 8)) & 0xff;  // :( :( :(
      m4_plot(xPos, yPos, color);
      cursor++;
    }
  }
}

inline void onVBlank(State& state) {
  if (state.isReady) {
    memcpy32(pal_bg_mem, PALETTE_BUFFER, sizeof(COLOR) * PALETTE_COLORS / 2);
    vid_flip();
  }

  state.isReady = false;
}

inline bool sync(State& state, u32 local, u32 remote) {
  bool wasVBlank = IS_VBLANK;

  while (spiSlave->transfer(local) != remote) {
    bool isVBlank = IS_VBLANK;
    if (!wasVBlank && isVBlank) {
      state.blindFrames++;
      wasVBlank = true;
    } else if (wasVBlank && !isVBlank)
      wasVBlank = false;

    if (state.blindFrames >= MAX_BLIND_FRAMES) {
      state.blindFrames = 0;
      return false;
    }
  }

  return true;
}

inline bool hasPixelChanged(u32 cursor) {
  uint32_t byte = cursor / 8;
  uint32_t bit = cursor % 8;

  return DIFFS_BUFFER[byte] >> bit & 1;
}

inline u32 x(u32 cursor) {
  return cursor % RENDER_WIDTH;
}

inline u32 y(u32 cursor) {
  return cursor / RENDER_WIDTH;
}
