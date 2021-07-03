#include <tonc.h>

#include "Benchmark.h"
#include "BuildConfig.h"
#include "Palette.h"
#include "Protocol.h"
#include "SPISlave.h"
#include "Utils.h"

extern "C" {
#include "gsmplayer/player.h"
}

#define VBLANK_TRACKER (pal_obj_mem[0])
#define TRY(ACTION) \
  if (!(ACTION))    \
    goto reset;

// -----
// STATE
// -----

typedef struct {
  u32 expectedPackets;
  u8 temporalDiffs[TEMPORAL_DIFF_SIZE];
  u8 paletteIndexByCompressedIndex[SPATIAL_DIFF_COLOR_LIMIT];
  u8 audioChunks[AUDIO_PADDED_SIZE];
  u32 iDiff, iPixel, pDiff, pPixel, pixelCount;
  u32 decompressCursor, diffCursor, pixelCursor;
  u32 currentJump;
  bool isSpatialCompressed;
  bool hasAudio;
  bool isAudioReady;
} State;

SPISlave* spiSlave = new SPISlave();
DATA_EWRAM u16 diffJumps[TOTAL_PIXELS];
DATA_EWRAM u8 compressedPixels[TOTAL_PIXELS];
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
bool sendKeysAndReceiveMetadata(State& state);
bool receiveAudio(State& state);
bool receivePixels(State& state);
void initProcessMetadata(State& state);
void initProcessPixels(State& state);
bool processMetadata(State& state);
bool processPixels(State& state);
bool isNewVBlank();
void driveAudio(State& state);
bool sync(State& state, u32 command);
u32 x(u32 cursor);
u32 y(u32 cursor);

// -----------
// DEFINITIONS
// -----------

template <typename F>
inline u32 transfer(State& state,
                    u32 packetToSend,
                    F doWhenIdle,
                    bool withRecovery = true) {
  bool breakFlag = false;
  u32 receivedPacket =
      spiSlave->transfer(packetToSend, isNewVBlank, &breakFlag, doWhenIdle);

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
  enableMosaic(DRAW_SCALE_X, DRAW_SCALE_Y);
  dma3_cpy(pal_bg_mem, MAIN_PALETTE, sizeof(COLOR) * PALETTE_COLORS);
  player_init();
}

CODE_IWRAM void mainLoop() {
  State state;
  state.isAudioReady = false;

reset:
  transfer(
      state, CMD_RESET, []() {}, false);

  while (true) {
    initProcessMetadata(state);
    TRY(sync(state, CMD_FRAME_START))
    TRY(sendKeysAndReceiveMetadata(state))
    if (state.hasAudio) {
      TRY(sync(state, CMD_AUDIO))
      TRY(receiveAudio(state))
    }
    while (!processMetadata(state))
      ;
    initProcessPixels(state);
    TRY(sync(state, CMD_PIXELS))
    TRY(receivePixels(state))
    if (state.expectedPackets > 0)
      while (!processPixels(state))
        ;
    TRY(sync(state, CMD_FRAME_END))
  }
}

inline bool sendKeysAndReceiveMetadata(State& state) {
  u16 keys = pressedKeys();
  u32 expectedPackets = spiSlave->transfer(keys);
  if (spiSlave->transfer(expectedPackets) != keys)
    return false;

  state.expectedPackets = expectedPackets & PACKS_BIT_MASK;
  state.isSpatialCompressed = (expectedPackets & COMPR_BIT_MASK) != 0;
  state.hasAudio = (expectedPackets & AUDIO_BIT_MASK) != 0;

  for (state.pDiff = 0; state.pDiff < TEMPORAL_DIFF_SIZE / PACKET_SIZE;
       state.pDiff++)
    ((u32*)state.temporalDiffs)[state.pDiff] =
        transfer(state, state.pDiff, [&state]() { processMetadata(state); });

  if (state.isSpatialCompressed)
    for (u32 i = 0; i < SPATIAL_DIFF_COLOR_LIMIT / PACKET_SIZE; i++)
      ((u32*)state.paletteIndexByCompressedIndex)[i] =
          transfer(state, i, [&state]() { processMetadata(state); });

  return true;
}

inline bool receiveAudio(State& state) {
  for (u32 i = 0; i < AUDIO_SIZE_PACKETS; i++)
    ((u32*)state.audioChunks)[i] =
        transfer(state, i, [&state]() { processMetadata(state); });

  state.isAudioReady = true;

  return true;
}

inline bool receivePixels(State& state) {
  for (state.pPixel = 0; state.pPixel < state.expectedPackets; state.pPixel++)
    ((u32*)compressedPixels)[state.pPixel] =
        transfer(state, state.pPixel, [&state]() { processPixels(state); });

  return true;
}

inline void initProcessMetadata(State& state) {
  state.decompressCursor = state.diffCursor = state.pixelCursor = 0;
  state.currentJump = 0;
  state.iDiff = 0;
}

inline void initProcessPixels(State& state) {
  state.decompressCursor = 0;
  state.pixelCount = state.diffCursor;
  state.diffCursor = 1;
  state.pixelCursor = diffJumps[0];
  state.iPixel = 0;
}

inline bool processMetadata(State& state) {
#define SAVE_JUMP(CHANGED_BIT)                       \
  state.currentJump++;                               \
  if ((CHANGED_BIT) != 0) {                          \
    diffJumps[state.diffCursor] = state.currentJump; \
    state.currentJump = 0;                           \
    state.diffCursor++;                              \
  }

  if (state.iDiff >= TEMPORAL_DIFF_SIZE)
    return true;
  else if (state.iDiff >= state.pDiff * PACKET_SIZE)
    return false;

  u8 byte = state.temporalDiffs[state.iDiff];

  if (byte == 0) {
    state.currentJump += 8 - (state.iDiff == 0);
    goto next;
  }

  if (state.iDiff > 0) {
    SAVE_JUMP(byte & (1 << 0))
  }
  SAVE_JUMP(byte & (1 << 1))
  SAVE_JUMP(byte & (1 << 2))
  SAVE_JUMP(byte & (1 << 3))
  SAVE_JUMP(byte & (1 << 4))
  SAVE_JUMP(byte & (1 << 5))
  SAVE_JUMP(byte & (1 << 6))
  SAVE_JUMP(byte & (1 << 7))

next:
  state.iDiff++;
  return false;
}

inline bool processPixels(State& state) {
#define HAS_FINISHED_PIXELS() state.diffCursor >= state.pixelCount

  if (HAS_FINISHED_PIXELS())
    return true;
  else if (state.iPixel >= state.pPixel * PACKET_SIZE)
    return false;

  u32 drawCursor, drawCursor32Bit;

#define DRAW_UNIQUE_PIXEL(PIXEL)                                           \
  drawCursor = y(state.pixelCursor) * DRAW_WIDTH + x(state.pixelCursor);   \
  drawCursor32Bit = drawCursor / 4;                                        \
  frameBuffer[drawCursor] =                                                \
      state.isSpatialCompressed                                            \
          ? state.paletteIndexByCompressedIndex[PIXEL &                    \
                                                ~SPATIAL_DIFF_COLOR_LIMIT] \
          : PIXEL;                                                         \
  ((u32*)vid_mem_front)[drawCursor32Bit] = ((u32*)frameBuffer)[drawCursor32Bit];

#define DRAW(PIXEL)                                                           \
  DRAW_UNIQUE_PIXEL(PIXEL);                                                   \
  if (state.isSpatialCompressed && (PIXEL & SPATIAL_DIFF_COLOR_LIMIT) != 0) { \
    state.pixelCursor++;                                                      \
    state.diffCursor++;                                                       \
    DRAW_UNIQUE_PIXEL(PIXEL)                                                  \
  }                                                                           \
  state.pixelCursor += diffJumps[state.diffCursor];                           \
  state.diffCursor++;

  u8 pixel = compressedPixels[state.iPixel];
  DRAW(pixel)
  state.iPixel++;

  return false;
}

inline bool isNewVBlank() {
  return false;  // TODO: RECOVER

  if (!VBLANK_TRACKER && IS_VBLANK) {
    VBLANK_TRACKER = true;
    return true;
  } else if (VBLANK_TRACKER && !IS_VBLANK)
    VBLANK_TRACKER = false;

  return false;
}

CODE_IWRAM void driveAudio(State& state) {
  if (player_needsData() && state.isAudioReady) {
    player_play((const unsigned char*)state.audioChunks, AUDIO_CHUNK_SIZE);
    state.isAudioReady = false;
  }

  spiSlave->stop();
  player_run();
  spiSlave->start();
}

inline bool sync(State& state, u32 command) {
  u32 local = command + CMD_GBA_OFFSET;
  u32 remote = command + CMD_RPI_OFFSET;
  bool wasVBlank = IS_VBLANK;

  while (true) {
    bool breakFlag = false;
    bool isOnSync =
        spiSlave->transfer(local, isNewVBlank, &breakFlag, []() {}) == remote;

    if (breakFlag) {
      driveAudio(state);
      continue;
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
