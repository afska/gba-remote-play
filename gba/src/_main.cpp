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
  u8 spatialDiffs[SPATIAL_DIFF_SIZE];
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
bool receiveSpatialDiffs(State& state);
bool receivePixels(State& state);
void draw(State& state);
void decompressImage(State& state);
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

    driveAudioIfNeeded();
    TRY(sync(CMD_FRAME_START));
    TRY(sendKeysAndReceiveTemporalDiffs(state));
    TRY(sync(CMD_SPATIAL_DIFFS_START));
    TRY(receiveSpatialDiffs(state));
    TRY(sync(CMD_PIXELS_START));
    TRY(receivePixels(state));
    TRY(sync(CMD_FRAME_END));
    driveAudioIfNeeded();

    draw(state);
  }
}

inline bool sendKeysAndReceiveTemporalDiffs(State& state) {
  state.expectedPackets = spiSlave->transfer(pressedKeys());

  for (u32 i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++)
    ((u32*)state.temporalDiffs)[i] = transfer(i);

  return true;
}

inline bool receiveSpatialDiffs(State& state) {
  for (u32 i = 0; i < SPATIAL_DIFF_SIZE / PACKET_SIZE; i++)
    ((u32*)state.spatialDiffs)[i] = transfer(i);

  return true;
}

inline bool receivePixels(State& state) {
  for (u32 i = 0; i < state.expectedPackets; i++)
    ((u32*)state.compressedPixels)[i] = transfer(i);

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
  u32 blindFrames = 0;
  bool wasVBlank = IS_VBLANK;

  while (true) {
    bool isOnSync = true;
    for (u32 i = 0; i < SYNC_VALIDATIONS; i++)
      isOnSync = isOnSync && transfer(local + i, false) == remote + i;

    if (isOnSync)
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
  return (cursor % RENDER_WIDTH) * DRAW_SCALE_X;
}

inline u32 y(u32 cursor) {
  return (cursor / RENDER_WIDTH) * DRAW_SCALE_Y;
}
