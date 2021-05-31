#include <tonc.h>

#include "Benchmark.h"
#include "Config.h"
#include "Protocol.h"
#include "SPISlave.h"

// -----
// STATE
// -----

SPISlave* spiSlave = new SPISlave();

typedef struct {
  u32 cursor = 0;
  bool isReady = false;
  u32 blindFrames = 0;
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
void receivePalette();
void receivePixels(State& state);
void onVBlank(State& state);
bool sync(State& state, u32 local, u32 remote);
u32 x(State& state);
u32 y(State& state);

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

  while (true) {
    spiSlave->transfer(CMD_RESET);

    if (!sync(state, CMD_FRAME_START_GBA, CMD_FRAME_START_RPI))
      continue;

    receivePalette();

    if (!sync(state, CMD_PIXELS_START_GBA, CMD_PIXELS_START_RPI))
      continue;

    receivePixels(state);

    sync(state, CMD_FRAME_END_GBA, CMD_FRAME_END_RPI);
    state.isReady = true;

    onVBlank(state);
  }
}

inline void receivePalette() {
  for (u32 i = 0; i < PALETTE_COLORS; i += COLORS_PER_PACKET) {
    u32 packet = spiSlave->transfer(0);
    pal_obj_mem[i] = packet & 0xffff;
    pal_obj_mem[i + 1] = (packet >> 16) & 0xffff;
  }
}

inline void receivePixels(State& state) {
  state.cursor = 0;
  u32 packet = 0;

  while ((packet = spiSlave->transfer(0)) != CMD_FRAME_END_RPI) {
    if (x(state) >= RENDER_WIDTH || y(state) >= RENDER_HEIGHT)
      break;

    u32 address = (y(state) * RENDER_WIDTH + x(state)) / PIXELS_PER_PACKET;
    ((u32*)vid_page)[address] = packet;
    state.cursor += PIXELS_PER_PACKET;
  }
}

inline void onVBlank(State& state) {
  if (state.isReady) {
    state.blindFrames = 0;
    memcpy32(pal_bg_mem, pal_obj_mem, sizeof(COLOR) * PALETTE_COLORS / 2);
    vid_flip();
  } else
    state.blindFrames++;

  state.isReady = false;
}

inline bool sync(State& state, u32 local, u32 remote) {
  bool wasVBlank = IS_VBLANK;

  while (spiSlave->transfer(local) != remote) {
    bool isVBlank = IS_VBLANK;
    if (!wasVBlank && isVBlank) {
      onVBlank(state);
      wasVBlank = true;
    } else if (wasVBlank && !isVBlank)
      wasVBlank = false;

    if (state.blindFrames >= MAX_BLIND_FRAMES) {
      state.isReady = false;
      state.blindFrames = 0;
      return false;
    }
  }

  return true;
}

inline u32 x(State& state) {
  return state.cursor % RENDER_WIDTH;
}

inline u32 y(State& state) {
  return state.cursor / RENDER_WIDTH;
}
