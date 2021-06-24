#include <tonc.h>

#include "Benchmark.h"
#include "Config.h"
#include "Palette.h"
#include "Protocol.h"
#include "SPISlave.h"

#define TRY(ACTION) \
  if (!(ACTION))    \
  goto reset

// -----
// STATE
// -----

typedef struct {
  u32 expectedPackets;
  u8 temporalDiffs[TEMPORAL_DIFF_SIZE];
  u8 compressedPixels[TOTAL_PIXELS];
} State;

SPISlave* spiSlave = new SPISlave();
DATA_EWRAM u8 frameBuffer[TOTAL_SCREEN_PIXELS];

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
bool sendKeysAndReceiveTemporalDiffs(State& state);
bool receivePixels(State& state);
void draw(State& state);
void decompressImage(State& state);
bool sync(u32 command);
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
  overclockEWRAM();
  if (DRAW_SCALE == 2)
    enable2xMosaic();
  dma3_cpy(pal_bg_mem, MAIN_PALETTE, sizeof(COLOR) * PALETTE_COLORS);
}

CODE_IWRAM void mainLoop() {
reset:
  State state;
  state.expectedPackets = 0;

  spiSlave->transfer(CMD_RESET);

  while (true) {
    state.expectedPackets = 0;

    TRY(sync(CMD_FRAME_START));
    TRY(sendKeysAndReceiveTemporalDiffs(state));
    TRY(sync(CMD_PIXELS_START));
    TRY(receivePixels(state));
    TRY(sync(CMD_FRAME_END));

    draw(state);
  }
}

inline bool sendKeysAndReceiveTemporalDiffs(State& state) {
  state.expectedPackets = spiSlave->transfer(0);

  u16 keys = pressedKeys();
  for (u32 i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++) {
    ((u32*)state.temporalDiffs)[i] =
        spiSlave->transfer(i < PRESSED_KEYS_REPETITIONS ? keys : i);
  }

  return true;
}

inline bool receivePixels(State& state) {
  for (u32 i = 0; i < state.expectedPackets; i++)
    ((u32*)state.compressedPixels)[i] = spiSlave->transfer(i);

  return true;
}

inline void draw(State& state) {
  decompressImage(state);
}

inline void decompressImage(State& state) {
  u32 decompressedPixels = 0;

  for (u32 cursor = 0; cursor < TOTAL_PIXELS; cursor++) {
    u32 temporalByte = cursor / 8;
    u32 temporalBit = cursor % 8;
    u32 temporalDiff = state.temporalDiffs[temporalByte];

    if (temporalBit == 0) {
      if (((u32*)state.temporalDiffs)[temporalByte / 4] == 0) {
        // (no changes in the next 32 pixels)
        cursor += 31;
        continue;
      } else if (((u16*)state.temporalDiffs)[temporalByte / 2] == 0) {
        // (no changes in the next 16 pixels)
        cursor += 15;
        continue;
      } else if (temporalDiff == 0) {
        // (no changes in the next 8 pixels)
        cursor += 7;
        continue;
      }
    }

    if ((temporalDiff >> temporalBit) & 1) {
      // (a pixel changed)

      u32 drawCursor =
          DRAW_SCALE == 1 ? cursor : y(cursor) * DRAW_WIDTH + x(cursor);
      u32 drawCursor32Bit = drawCursor / 4;
      frameBuffer[drawCursor] = state.compressedPixels[decompressedPixels];
      ((u32*)vid_mem_front)[drawCursor32Bit] =
          ((u32*)frameBuffer)[drawCursor32Bit];

      decompressedPixels++;
    }
  }
}

inline bool sync(u32 command) {
  u32 local = command + CMD_GBA_OFFSET;
  u32 remote = command + CMD_RPI_OFFSET;
  u32 blindFrames = 0;
  bool wasVBlank = IS_VBLANK;

  while (true) {
    if (spiSlave->transfer(local) == remote)
      return true;
    else {
      bool isVBlank = IS_VBLANK;

      if (!wasVBlank && isVBlank) {
        blindFrames++;
        wasVBlank = true;
      } else if (wasVBlank && !isVBlank)
        wasVBlank = false;

      if (blindFrames >= MAX_BLIND_FRAMES)
        return false;
    }
  }
}

inline u32 x(u32 cursor) {
  return (cursor % RENDER_WIDTH) * DRAW_SCALE;
}

inline u32 y(u32 cursor) {
  return (cursor / RENDER_WIDTH) * DRAW_SCALE;
}
