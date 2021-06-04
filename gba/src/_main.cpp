#include <tonc.h>

#include "Benchmark.h"
#include "Config.h"
#include "HammingWeight.h"
#include "Protocol.h"
#include "SPISlave.h"

// -----
// STATE
// -----

SPISlave* spiSlave = new SPISlave();
DATA_EWRAM u8 colorIndexBuffer[GBA_MAX_COLORS];

typedef struct {
  u32 blindFrames;
  u32 expectedPixels;
  COLOR palette[PALETTE_COLORS];
  u8 diffs[TEMPORAL_DIFF_SIZE];
  u16* lastBuffer;
  bool isReadyToDraw;
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
void receivePalette(State& state);
void receivePixels(State& state);
void onVBlank(State& state);
void decompressImage(State& state);
bool sync(State& state, u32 local, u32 remote);
bool hasPixelChanged(State& state, u32 cursor);
u32 addressOf(u32 cursor);
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
  enableMode4AndBackground2();
  overclockIWRAM();
  enable2xMosaic();
}

CODE_IWRAM void mainLoop() {
reset:
  State state;
  state.blindFrames = 0;
  state.expectedPixels = 0;
  state.lastBuffer = (u16*)vid_page;
  state.isReadyToDraw = false;
  spiSlave->transfer(CMD_RESET);

  while (true) {
    if (state.isReadyToDraw) {
      if (IS_VBLANK)
        onVBlank(state);
      else
        continue;
    }

    state.blindFrames = 0;
    state.expectedPixels = 0;

    if (!sync(state, CMD_FRAME_START_GBA, CMD_FRAME_START_RPI))
      goto reset;

    receiveDiffs(state);

    if (!sync(state, CMD_PALETTE_START_GBA, CMD_PALETTE_START_RPI))
      goto reset;

    receivePalette(state);

    if (!sync(state, CMD_PIXELS_START_GBA, CMD_PIXELS_START_RPI))
      goto reset;

    receivePixels(state);

    if (!sync(state, CMD_FRAME_END_GBA, CMD_FRAME_END_RPI))
      goto reset;

    state.isReadyToDraw = true;
  }
}

inline void receiveDiffs(State& state) {
  for (u32 i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++)
    ((u32*)state.diffs)[i] = spiSlave->transfer(0);

  for (u32 i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++)
    state.expectedPixels += HammingWeight(((u32*)state.diffs)[i]);
}

inline void receivePalette(State& state) {
  for (u32 i = 0; i < PALETTE_COLORS; i += COLORS_PER_PACKET)
    ((u32*)state.palette)[i / 2] = spiSlave->transfer(0);

  for (u32 i = 0; i < PALETTE_COLORS; i += COLORS_PER_PACKET) {
    u32 packet = ((u32*)state.palette)[i / 2];
    colorIndexBuffer[FIRST_COLOR(packet)] = i;
    colorIndexBuffer[SECOND_COLOR(packet)] = i + 1;
  }
}

inline void receivePixels(State& state) {
  u32 cursor = 0;
  u32 packet = 0;

  while (cursor < state.expectedPixels) {
    packet = spiSlave->transfer(0);
    ((u32*)vid_page)[addressOf(cursor)] = packet;
    cursor += PIXELS_PER_PACKET;
  }

  decompressImage(state);
}

inline void onVBlank(State& state) {
  dma3_cpy(pal_bg_mem, state.palette, sizeof(COLOR) * PALETTE_COLORS);
  state.lastBuffer = (u16*)vid_page;
  vid_flip();
  state.isReadyToDraw = false;
}

inline void decompressImage(State& state) {
  u32 compressedBufferEnd = state.expectedPixels - 1;
  for (int cursor = TOTAL_PIXELS - 1; cursor >= 0; cursor--) {
    if (hasPixelChanged(state, cursor)) {
      m4_plot(x(cursor), y(cursor), m4Get(compressedBufferEnd));
      compressedBufferEnd--;
    } else {
      u8 oldColorIndex = m4GetXYFrom(state.lastBuffer, x(cursor), y(cursor));
      COLOR repeatedColor = pal_bg_mem[oldColorIndex];
      m4_plot(x(cursor), y(cursor), colorIndexBuffer[repeatedColor]);
    }
  }
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

  state.blindFrames = 0;
  return true;
}

inline bool hasPixelChanged(State& state, u32 cursor) {
  uint32_t byte = cursor / 8;
  uint8_t bit = cursor % 8;

  return (state.diffs[byte] >> bit) & 1;
}

inline u32 addressOf(u32 cursor) {
  return cursor / PIXELS_PER_PACKET;
}

inline u32 x(u32 cursor) {
  return (cursor % RENDER_WIDTH) * RENDER_SCALE;
}

inline u32 y(u32 cursor) {
  return (cursor / RENDER_WIDTH) * RENDER_SCALE;
}
