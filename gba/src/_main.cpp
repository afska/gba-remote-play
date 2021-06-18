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
  u8 spatialDiffs[SPATIAL_DIFF_SIZE];
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
bool receiveSpatialDiffs(State& state);
bool receivePixels(State& state);
void draw(State& state);
void decompressImage(State& state);
bool sync(u32 command);
bool reliablySend(u32 packetToSend, u32 expectedResponse);
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
    TRY(sync(CMD_SPATIAL_DIFFS_START));
    TRY(receiveSpatialDiffs(state));
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

inline bool receiveSpatialDiffs(State& state) {
  for (u32 i = 0; i < SPATIAL_DIFF_SIZE / PACKET_SIZE; i++)
    ((u32*)state.spatialDiffs)[i] = spiSlave->transfer(i);

  return true;
}

inline bool receivePixels(State& state) {
  for (u32 i = 0; i < state.expectedPackets; i++) {
    // TODO: TEST CODE, DELETE
    if (i % TRANSFER_SYNC_FREQUENCY == 0 && REG_VCOUNT == 5) {
      if (!sync(CMD_PAUSE))
        return false;

      vid_vsync();
      vid_vsync();

      if (!sync(CMD_RESUME))
        return false;
    }

    ((u32*)state.compressedPixels)[i] = spiSlave->transfer(i);
  }

  return true;
}

inline void draw(State& state) {
  decompressImage(state);
  dma3_cpy(vid_mem_front, frameBuffer, TOTAL_SCREEN_PIXELS);
}

inline void decompressImage(State& state) {
  u32 block = 0;
  u32 blockPart = 0;
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

      u32 spatialByte = block / 8;
      u32 spatialBit = block % 8;
      u32 spatialDiff = state.spatialDiffs[spatialByte];
      bool isRepeatedBlock = !((spatialDiff >> spatialBit) & 1);

      u32 drawCursor = y(cursor) * DRAW_WIDTH + x(cursor);
      frameBuffer[drawCursor] = state.compressedPixels[decompressedPixels];

      blockPart++;
      if (blockPart == SPATIAL_DIFF_BLOCK_SIZE) {
        block++;
        blockPart = 0;
        decompressedPixels++;
      } else if (!isRepeatedBlock)
        decompressedPixels++;
    }
  }
}

inline bool sync(u32 command) {
  u32 local = command + CMD_GBA_OFFSET;
  u32 remote = command + CMD_RPI_OFFSET;

  return reliablySend(local, remote);
}

inline bool reliablySend(u32 packetToSend, u32 expectedResponse) {
  u32 blindFrames = 0;
  bool wasVBlank = IS_VBLANK;

  while (spiSlave->transfer(packetToSend) != expectedResponse) {
    bool isVBlank = IS_VBLANK;

    if (!wasVBlank && isVBlank) {
      blindFrames++;
      wasVBlank = true;
    } else if (wasVBlank && !isVBlank)
      wasVBlank = false;

    if (blindFrames >= MAX_BLIND_FRAMES)
      return false;
  }

  return true;
}

inline u32 x(u32 cursor) {
  return (cursor % RENDER_WIDTH) * DRAW_SCALE_X;
}

inline u32 y(u32 cursor) {
  return (cursor / RENDER_WIDTH) * DRAW_SCALE_Y;
}
