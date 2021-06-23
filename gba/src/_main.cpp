#include <tonc.h>

#include "Benchmark.h"
#include "Config.h"
#include "Palette.h"
#include "Protocol.h"
#include "SPISlave.h"

extern "C" {
#include "gbfs/gbfs.h"
#include "gsmplayer/player.h"
}

#define VBLANK_TRACKER (pal_obj_mem[0])
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
static const GBFS_FILE* fs = find_first_gbfs_file(0);

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
void render(State& state);
u32 transfer(u32 packetToSend, bool withRecovery = true);
void driveAudioIfNeeded();
bool isNewVBlank();
void driveAudio();
bool sync(u32 command);
u32 x(u32 cursor);
u32 y(u32 cursor);

// -----------
// DEFINITIONS
// -----------

#ifndef BENCHMARK
int main() {
  init();
  player_loop("loop.gsm");
  mainLoop();

  return 0;
}
#endif

inline void init() {
  enableMode4AndBackground2();
  overclockEWRAM();
  enable2xMosaic();
  dma3_cpy(pal_bg_mem, MAIN_PALETTE, sizeof(COLOR) * PALETTE_COLORS);

  if (fs == NULL) {
    pal_bg_mem[0] = 0b11111;
    while (true)
      ;
  }

  player_init();
}

CODE_IWRAM void mainLoop() {
reset:
  State state;
  transfer(CMD_RESET, false);

  while (true) {
    state.expectedPackets = 0;

    TRY(sync(CMD_FRAME_START));
    TRY(sendKeysAndReceiveTemporalDiffs(state));
    TRY(receivePixels(state));

    render(state);
  }
}

inline bool sendKeysAndReceiveTemporalDiffs(State& state) {
  u16 keys = pressedKeys();
  state.expectedPackets = spiSlave->transfer(keys);
  driveAudioIfNeeded();
  if (spiSlave->transfer(keys + 1) != state.expectedPackets + 1)
    return false;
  driveAudioIfNeeded();

  for (u32 i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++)
    ((u32*)state.temporalDiffs)[i] = transfer(i);

  return true;
}

inline bool receivePixels(State& state) {
  for (u32 i = 0; i < state.expectedPackets; i++)
    ((u32*)state.compressedPixels)[i] = transfer(i);

  return true;
}

inline void render(State& state) {
  u32 decompressedPixels = 0;

  for (u32 cursor = 0; cursor < TOTAL_PIXELS; cursor++) {
    driveAudioIfNeeded();

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

      u32 drawCursor = y(cursor) * DRAW_WIDTH + x(cursor);
      frameBuffer[drawCursor] = state.compressedPixels[decompressedPixels];
      ((u32*)vid_mem_front)[drawCursor / 4] =
          ((u32*)frameBuffer)[drawCursor / 4];

      decompressedPixels++;
    }
  }
}

inline u32 transfer(u32 packetToSend, bool withRecovery) {
  bool breakFlag = false;
  u32 receivedPacket =
      spiSlave->transfer(packetToSend, isNewVBlank, &breakFlag);

  if (breakFlag) {
    driveAudio();

    if (withRecovery) {
      sync(CMD_RECOVERY);
      spiSlave->transfer(packetToSend);
      receivedPacket = spiSlave->transfer(packetToSend);
    }
  }

  return receivedPacket;
}

inline void driveAudioIfNeeded() {
  if (isNewVBlank())
    driveAudio();
}

inline bool isNewVBlank() {
  bool isVBlank = IS_VBLANK;
  if (!VBLANK_TRACKER && isVBlank) {
    VBLANK_TRACKER = true;
    return true;
  } else if (VBLANK_TRACKER && !isVBlank)
    VBLANK_TRACKER = false;

  return false;
}

CODE_IWRAM void driveAudio() {
  player_run();
}

inline bool sync(u32 command) {
  u32 local = command + CMD_GBA_OFFSET;
  u32 remote = command + CMD_RPI_OFFSET;
  bool wasVBlank = IS_VBLANK;

  while (true) {
    bool breakFlag = false;
    bool isOnSync =
        spiSlave->transfer(local, isNewVBlank, &breakFlag) == remote;

    if (breakFlag) {
      driveAudio();
      return false;
    }

    if (isOnSync)
      return true;
    else {
      bool isVBlank = IS_VBLANK;

      if (!wasVBlank && isVBlank)
        wasVBlank = true;
      else if (wasVBlank && !isVBlank)
        return false;
    }
  }
}

inline u32 x(u32 cursor) {
  return (cursor % RENDER_WIDTH) * DRAW_SCALE_X;
}

inline u32 y(u32 cursor) {
  return (cursor / RENDER_WIDTH) * DRAW_SCALE_Y;
}
