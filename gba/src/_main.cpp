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
  u8 audioChunks[AUDIO_BUFFERS][AUDIO_PADDED_SIZE];
  u8 audioChunkIndex;
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
bool receiveAudio(State& state);
bool receivePixels(State& state);
void render(State& state);
void driveAudioIfNeeded(State& state);
bool isNewVBlank();
void driveAudio(State& state);
u32 transfer(State& state, u32 packetToSend, bool withRecovery = true);
bool sync(State& state, u32 command);
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

  // TODO: REMOVE GBFS
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
  state.audioChunkIndex = 0;
  transfer(state, CMD_RESET, false);

  while (true) {
    state.expectedPackets = 0;

    TRY(sync(state, CMD_FRAME_START));
    TRY(sendKeysAndReceiveTemporalDiffs(state));
    TRY(receiveAudio(state));
    TRY(receivePixels(state));

    render(state);
  }
}

inline bool sendKeysAndReceiveTemporalDiffs(State& state) {
  u16 keys = pressedKeys();
  state.expectedPackets = spiSlave->transfer(keys);
  driveAudioIfNeeded(state);
  if (spiSlave->transfer(keys + 1) != state.expectedPackets + 1)
    return false;
  driveAudioIfNeeded(state);

  for (u32 i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++)
    ((u32*)state.temporalDiffs)[i] = transfer(state, i);

  return true;
}

inline bool receiveAudio(State& state) {
  for (u32 i = 0; i < AUDIO_SIZE_PACKETS; i++)
    ((u32*)state.audioChunks[state.audioChunkIndex])[i] = transfer(state, i);

  return true;
}

inline bool receivePixels(State& state) {
  for (u32 i = 0; i < state.expectedPackets; i++)
    ((u32*)state.compressedPixels)[i] = transfer(state, i);

  return true;
}

inline void render(State& state) {
  u32 decompressedPixels = 0;

  for (u32 cursor = 0; cursor < TOTAL_PIXELS; cursor++) {
    driveAudioIfNeeded(state);

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
      u32 drawCursor32Bit = drawCursor / 4;
      frameBuffer[drawCursor] = state.compressedPixels[decompressedPixels];
      ((u32*)vid_mem_front)[drawCursor32Bit] =
          ((u32*)frameBuffer)[drawCursor32Bit];

      decompressedPixels++;
    }
  }
}

inline void driveAudioIfNeeded(State& state) {
  if (isNewVBlank())
    driveAudio(state);
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

CODE_IWRAM void driveAudio(State& state) {
  player_play((char*)state.audioChunks[state.audioChunkIndex]);
  player_run();
  state.audioChunkIndex = !state.audioChunkIndex;
}

inline u32 transfer(State& state, u32 packetToSend, bool withRecovery) {
  bool breakFlag = false;
  u32 receivedPacket =
      spiSlave->transfer(packetToSend, isNewVBlank, &breakFlag);

  if (breakFlag) {
    driveAudio(state);

    if (withRecovery) {
      sync(state, CMD_RECOVERY);
      spiSlave->transfer(packetToSend);
      receivedPacket = spiSlave->transfer(packetToSend);
    }
  }

  return receivedPacket;
}

inline bool sync(State& state, u32 command) {
  u32 local = command + CMD_GBA_OFFSET;
  u32 remote = command + CMD_RPI_OFFSET;
  bool wasVBlank = IS_VBLANK;

  while (true) {
    bool breakFlag = false;
    bool isOnSync =
        spiSlave->transfer(local, isNewVBlank, &breakFlag) == remote;

    if (breakFlag) {
      driveAudio(state);
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
