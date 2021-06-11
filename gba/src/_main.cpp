#include <tonc.h>

#include "Benchmark.h"
#include "Config.h"
#include "Palette.h"
#include "Protocol.h"
#include "SPISlave.h"

// -----
// STATE
// -----

typedef struct {
  u32 expectedPixels;
  u8 diffs[TEMPORAL_DIFF_SIZE];
  u8* frameBuffer;
  u8* lastFrameBuffer;
} State;

SPISlave* spiSlave = new SPISlave();
DATA_EWRAM u8 frameBufferA[TOTAL_SCREEN_PIXELS];
DATA_EWRAM u8 frameBufferB[TOTAL_SCREEN_PIXELS];

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
void receivePixels(State& state);
void draw(State& state);
void decompressImage(State& state);
bool sync(State& state, u32 local, u32 remote);
bool hasPixelChanged(State& state, u32 cursor);
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
  state.expectedPixels = 0;
  state.frameBuffer = (u8*)frameBufferA;
  state.lastFrameBuffer = (u8*)frameBufferB;

  spiSlave->transfer(CMD_RESET);

  while (true) {
    state.expectedPixels = 0;

    if (!sync(state, CMD_FRAME_START_GBA, CMD_FRAME_START_RPI))
      goto reset;

    receiveDiffs(state);

    if (!sync(state, CMD_PIXELS_START_GBA, CMD_PIXELS_START_RPI))
      goto reset;

    receivePixels(state);

    if (!sync(state, CMD_FRAME_END_GBA, CMD_FRAME_END_RPI))
      goto reset;

    draw(state);
  }
}

inline void receiveDiffs(State& state) {
  u16 keys = pressedKeys();
  for (u32 i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++)
    ((u32*)state.diffs)[i] = spiSlave->transfer(keys);
  state.expectedPixels = spiSlave->transfer(0);
}

inline void receivePixels(State& state) {
  u32 expectedPackets = state.expectedPixels / PIXELS_PER_PACKET +
                        state.expectedPixels % PIXELS_PER_PACKET;
  for (u32 i = 0; i < expectedPackets; i++)
    ((u32*)state.frameBuffer)[i] = spiSlave->transfer(0);
}

inline void draw(State& state) {
  decompressImage(state);
  dma3_cpy(vid_page, state.frameBuffer, TOTAL_SCREEN_PIXELS);
  vid_flip();

  auto drawnFrameBuffer = state.frameBuffer;
  state.frameBuffer = state.lastFrameBuffer;
  state.lastFrameBuffer = drawnFrameBuffer;
}

inline void decompressImage(State& state) {
  u32 compressedBufferEnd = state.expectedPixels - 1;
  for (int cursor = TOTAL_PIXELS - 1; cursor >= 0; cursor--) {
    u8 xPos = x(cursor);
    u8 yPos = y(cursor);
    u32 drawCursor = yPos * DRAW_WIDTH + xPos;

    if (hasPixelChanged(state, cursor)) {
      state.frameBuffer[drawCursor] = state.frameBuffer[compressedBufferEnd];
      compressedBufferEnd--;
    } else
      state.frameBuffer[drawCursor] = state.lastFrameBuffer[drawCursor];
  }
}

inline bool sync(State& state, u32 local, u32 remote) {
  u32 blindFrames = 0;
  bool wasVBlank = IS_VBLANK;

  while (spiSlave->transfer(local) != remote) {
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

inline bool hasPixelChanged(State& state, u32 cursor) {
  uint32_t byte = cursor / 8;
  uint8_t bit = cursor % 8;

  return (state.diffs[byte] >> bit) & 1;
}

inline u32 x(u32 cursor) {
  return (cursor % RENDER_WIDTH) * DRAW_SCALE_X;
}

inline u32 y(u32 cursor) {
  return (cursor / RENDER_WIDTH) * DRAW_SCALE_Y;
}
