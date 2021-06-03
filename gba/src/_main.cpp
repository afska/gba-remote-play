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

enum Step {
  SYNCING_START,
  RECEIVING_DIFFS,
  SYNCING_PALETTE,
  RECEIVING_PALETTE,
  SYNCING_PIXELS,
  RECEIVING_PIXELS,
  SYNCING_END,
  READY_TO_DRAW
};

typedef struct {
  u32 blindFrames;
  u32 expectedPixels;
  COLOR palette[PALETTE_COLORS];
  u8 diffs[TEMPORAL_DIFF_SIZE];
  u8* lastBuffer;
  Step step;
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
  ENABLE_MODE4_AND_BG2();
  OVERCLOCK_IWRAM();
}

CODE_IWRAM void mainLoop() {
reset:
  State state;
  state.blindFrames = 0;
  state.expectedPixels = 0;
  state.lastBuffer = (u8*)vid_page;
  state.step = Step::SYNCING_START;
  spiSlave->transfer(CMD_RESET);

  while (true) {
    switch (state.step) {
      case Step::SYNCING_START: {
        state.blindFrames = 0;
        state.expectedPixels = 0;
        if (!sync(state, CMD_FRAME_START_GBA, CMD_FRAME_START_RPI))
          goto reset;

        state.step = Step::RECEIVING_DIFFS;
        break;
      }
      case Step::RECEIVING_DIFFS: {
        receiveDiffs(state);

        state.step = Step::SYNCING_PALETTE;
        break;
      }
      case Step::SYNCING_PALETTE: {
        if (!sync(state, CMD_PALETTE_START_GBA, CMD_PALETTE_START_RPI))
          goto reset;

        state.step = Step::RECEIVING_PALETTE;
        break;
      }
      case Step::RECEIVING_PALETTE: {
        receivePalette(state);

        state.step = Step::SYNCING_PIXELS;
        break;
      }
      case Step::SYNCING_PIXELS: {
        if (!sync(state, CMD_PIXELS_START_GBA, CMD_PIXELS_START_RPI))
          goto reset;

        state.step = Step::RECEIVING_PIXELS;
        break;
      }
      case Step::RECEIVING_PIXELS: {
        receivePixels(state);

        state.step = Step::SYNCING_END;
        break;
      }
      case Step::SYNCING_END: {
        if (!sync(state, CMD_FRAME_END_GBA, CMD_FRAME_END_RPI))
          goto reset;

        state.step = Step::READY_TO_DRAW;
        break;
      }
      case Step::READY_TO_DRAW: {
        if (IS_VBLANK) {
          onVBlank(state);
          state.step = Step::SYNCING_START;
        }

        break;
      }
    }
  }
}

inline void receiveDiffs(State& state) {
  for (u32 i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++) {
    u32 packet = spiSlave->transfer(0);
    ((u32*)state.diffs)[i] = packet;
    state.expectedPixels += HammingWeight(packet);
  }
}

inline void receivePalette(State& state) {
  for (u32 i = 0; i < PALETTE_COLORS; i += COLORS_PER_PACKET) {
    u32 packet = spiSlave->transfer(0);
    state.palette[i] = packet & 0xffff;  // TODO: RECEIVE PALETTE AS U32?
    state.palette[i + 1] = (packet >> 16) & 0xffff;
    colorIndexBuffer[packet & 0xffff] = i;
    colorIndexBuffer[(packet >> 16) & 0xffff] = i + 1;
  }
}

inline void receivePixels(State& state) {
  u32 cursor = 0;
  u32 packet = 0;
  u32 byte = PIXELS_PER_PACKET;

  while (cursor < TOTAL_PIXELS) {
    if (byte == PIXELS_PER_PACKET) {
      packet = spiSlave->transfer(0);
      byte = 0;
    }

    if (hasPixelChanged(state, cursor)) {
      u8 newColorIndex = (packet >> (byte * 8)) & 0xff;
      m4_plot(x(cursor), y(cursor), newColorIndex);
      byte++;
    } else {
      u8 oldColorIndex = state.lastBuffer[cursor];
      COLOR repeatedColor = pal_bg_mem[oldColorIndex];
      m4_plot(x(cursor), y(cursor), colorIndexBuffer[repeatedColor]);
    }

    cursor++;
  }
}

inline void onVBlank(State& state) {
  memcpy32(pal_bg_mem, state.palette, sizeof(COLOR) * PALETTE_COLORS / 2);
  state.lastBuffer = (u8*)vid_page;
  vid_flip();
  state.step = Step::SYNCING_START;
}

CODE_IWRAM bool sync(State& state, u32 local, u32 remote) {
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

inline bool hasPixelChanged(State& state, u32 cursor) {
  uint32_t byte = cursor / 8;
  uint8_t bit = cursor % 8;

  return (state.diffs[byte] >> bit) & 1;
}

inline u32 x(u32 cursor) {
  return cursor % RENDER_WIDTH;
}

inline u32 y(u32 cursor) {
  return cursor / RENDER_WIDTH;
}
