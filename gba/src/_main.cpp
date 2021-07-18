#include <tonc.h>

#include "Benchmark.h"
#include "BuildConfig.h"
#include "Palette.h"
#include "Protocol.h"
#include "RuntimeConfig.h"
#include "Utils.h"
#include "_state.h"

extern "C" {
#include "gsmplayer/player.h"
}

#define TRY(ACTION) \
  if (!(ACTION))    \
    goto reset;

// ------------
// DECLARATIONS
// ------------

void wipeScreen();
void init();
void mainLoop();
void syncReset();
bool sendKeysAndReceiveMetadata();
bool receiveAudio();
bool receivePixels();
void render(bool withRLE, u32 width, u32 scaleX, u32 scaleY, u32 totalPixels);
bool needsToRunAudio();
void runAudio();
u32 transfer(u32 packetToSend, bool withRecovery = true);
bool sync(u32 command);
u32 x(u32 cursor, u32 width, u32 scaleX);
u32 y(u32 cursor, u32 width, u32 scaleY);
void optimizedRender();

// -----------
// DEFINITIONS
// -----------

CODE_EWRAM int main() {
  RuntimeConfig::initialize();

  while (true) {
    RuntimeConfig::show();
    wipeScreen();

    if (config.isBenchmark()) {
      syncReset();
      Benchmark::init();
      Benchmark::mainLoop();
    }

    init();
    mainLoop();

#ifdef WITH_AUDIO
    player_stop();
#endif
  }

  return 0;
}

ALWAYS_INLINE void wipeScreen() {
  tte_erase_screen();
  for (u32 i = 0; i < TOTAL_SCREEN_PIXELS; i++)
    m4Draw(i, 0);
  setMosaic(1, 1);
}

ALWAYS_INLINE void init() {
  enableMode4AndBackground2();
  overclockEWRAM();
  setMosaic(RENDER_MODE_SCALEX[config.renderMode],
            config.scanlines ? 1 : RENDER_MODE_SCALEY[config.renderMode]);
  dma3_cpy(pal_bg_mem, MAIN_PALETTE, sizeof(COLOR) * PALETTE_COLORS);
#ifdef WITH_AUDIO
  player_init();
#endif
}

CODE_IWRAM void mainLoop() {
  state.hasAudio = false;
  state.isVBlank = false;
  state.isAudioReady = false;

reset:
  syncReset();

  while (true) {
    if (needsRestart())
      return;

    TRY(sync(CMD_FRAME_START))
    TRY(sendKeysAndReceiveMetadata())
    if (state.hasAudio) {
      TRY(sync(CMD_AUDIO))
      TRY(receiveAudio())
    }
    TRY(sync(CMD_PIXELS))
    TRY(receivePixels())
    TRY(sync(CMD_FRAME_END))

    optimizedRender();
  }
}

ALWAYS_INLINE void syncReset() {
  u32 resetPacket = CMD_RESET + (config.renderMode |
                                 (config.controls << CONTROLS_BIT_OFFSET));
  while (transfer(resetPacket, false) != resetPacket)
    ;
}

ALWAYS_INLINE bool sendKeysAndReceiveMetadata() {
  u16 keys = pressedKeys();
  u32 metadata = spiSlave->transfer(keys);
  if (spiSlave->transfer(metadata) != keys)
    return false;

  state.expectedPackets = (metadata >> PACKS_BIT_OFFSET) & PACKS_BIT_MASK;
  state.startPixel = metadata & START_BIT_MASK;
  state.isRLE = (metadata & COMPR_BIT_MASK) != 0;
  state.hasAudio = (metadata & AUDIO_BIT_MASK) != 0;

  u32 diffMaxPackets =
      TEMPORAL_DIFF_MAX_PACKETS(RENDER_MODE_PIXELS[config.renderMode]);
  u32 diffStart = (state.startPixel / 8) / PACKET_SIZE;
  u32 diffEndPacket = min(spiSlave->transfer(0), diffMaxPackets);
  for (u32 i = diffStart; i < diffEndPacket; i++)
    ((u32*)state.temporalDiffs)[i] = transfer(i);
  for (u32 i = diffEndPacket; i < diffMaxPackets; i++)
    ((u32*)state.temporalDiffs)[i] = 0;

  return true;
}

ALWAYS_INLINE bool receiveAudio() {
  for (u32 i = 0; i < AUDIO_SIZE_PACKETS; i++)
    ((u32*)state.audioChunks)[i] = transfer(i);

  state.isAudioReady = true;

  return true;
}

ALWAYS_INLINE bool receivePixels() {
  for (u32 i = 0; i < state.expectedPackets; i++)
    ((u32*)compressedPixels)[i] = transfer(i);

  return true;
}

ALWAYS_INLINE void render(bool withRLE,
                          u32 width,
                          u32 scaleX,
                          u32 scaleY,
                          u32 totalPixels) {
  u32 cursor = state.startPixel;
  u32 rleRepeats = compressedPixels[0];
  u32 decompressedBytes = withRLE;

#define RUN_AUDIO_IF_NEEDED()               \
  if (withRLE) {                            \
    if (needsToRunAudio())                  \
      runAudio();                           \
  } else {                                  \
    if (!(cursor % 8) && needsToRunAudio()) \
      runAudio();                           \
  }
#define DRAW_PIXEL(PIXEL)                                                  \
  m4Draw(y(cursor, width, scaleY) * DRAW_WIDTH + x(cursor, width, scaleX), \
         PIXEL);
#define DRAW_NEXT()                                         \
  if (withRLE) {                                            \
    u8 pixel = compressedPixels[decompressedBytes];         \
    if (--rleRepeats == 0) {                                \
      rleRepeats = compressedPixels[decompressedBytes + 1]; \
      decompressedBytes += 2;                               \
    }                                                       \
    DRAW_PIXEL(pixel)                                       \
    cursor++;                                               \
  } else {                                                  \
    u8 pixel = compressedPixels[decompressedBytes];         \
    DRAW_PIXEL(pixel);                                      \
    decompressedBytes++;                                    \
    cursor++;                                               \
  }
#define DRAW_BATCH(TIMES)                        \
  u32 target = min(cursor + TIMES, totalPixels); \
  while (cursor < target) {                      \
    RUN_AUDIO_IF_NEEDED()                        \
    DRAW_NEXT()                                  \
  }

  while (cursor < totalPixels) {
    RUN_AUDIO_IF_NEEDED()
    u32 diffCursor = cursor / 8;
    u32 diffCursorBit = cursor % 8;
    if (diffCursorBit == 0) {
      u32 diffWord, diffHalfWord, diffByte;

      if ((diffWord = ((u32*)state.temporalDiffs)[diffCursor / 4]) == 0) {
        // (none of the next 32 pixels changed)
        cursor += 32;
        continue;
      } else if (diffWord == 0xffffffff) {
        // (all next 32 pixels changed)
        DRAW_BATCH(32)
        continue;
      } else if ((diffHalfWord = ((u16*)state.temporalDiffs)[diffCursor / 2]) ==
                 0) {
        // (none of the next 16 pixels changed)
        cursor += 16;
        continue;
      } else if (diffHalfWord == 0xffff) {
        // (all next 16 pixels changed)
        DRAW_BATCH(16)
        continue;
      } else if ((diffByte = state.temporalDiffs[diffCursor]) == 0) {
        // (none of the 8 pixels changed)
        cursor += 8;
        continue;
      } else if (diffByte == 0xff) {
        // (all next 8 pixels changed)
        DRAW_BATCH(8)
        continue;
      }
    }

    u8 diffByte = state.temporalDiffs[diffCursor];
    if (BIT_IS_HIGH(diffByte, diffCursorBit)) {
      // (a pixel changed)
      DRAW_NEXT()
    } else
      cursor++;
  }
}

ALWAYS_INLINE bool needsToRunAudio() {
#ifndef WITH_AUDIO
  return false;
#endif

  if (!state.isVBlank && IS_VBLANK) {
    state.isVBlank = true;
    return true;
  } else if (state.isVBlank && !IS_VBLANK)
    state.isVBlank = false;

  return false;
}

CODE_IWRAM void runAudio() {
  if (player_needsData() && state.isAudioReady) {
    player_play((const unsigned char*)state.audioChunks, AUDIO_CHUNK_SIZE);
    state.isAudioReady = false;
  }

  spiSlave->stop();
  player_run();
  spiSlave->start();
}

ALWAYS_INLINE u32 transfer(u32 packetToSend, bool withRecovery) {
  bool breakFlag = false;
  u32 receivedPacket =
      spiSlave->transfer(packetToSend, needsToRunAudio, &breakFlag);

  if (breakFlag) {
    runAudio();

    if (withRecovery) {
      sync(CMD_RECOVERY);
      spiSlave->transfer(packetToSend);
      receivedPacket = spiSlave->transfer(packetToSend);
    }
  }

  return receivedPacket;
}

CODE_IWRAM bool sync(u32 command) {
  u32 local = command + CMD_GBA_OFFSET;
  u32 remote = command + CMD_RPI_OFFSET;
  bool wasVBlank = IS_VBLANK;

  while (true) {
    bool breakFlag = false;
    bool isOnSync =
        spiSlave->transfer(local, needsToRunAudio, &breakFlag) == remote;

    if (breakFlag) {
      runAudio();
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

ALWAYS_INLINE u32 x(u32 cursor, u32 width, u32 scaleX) {
  return (cursor % width) * scaleX;
}

ALWAYS_INLINE u32 y(u32 cursor, u32 width, u32 scaleY) {
  return (cursor / width) * scaleY;
}

ALWAYS_INLINE void optimizedRender() {
#define RENDER(N, WITH_RLE)                                     \
  render(WITH_RLE, RENDER_MODE_WIDTH[N], RENDER_MODE_SCALEX[N], \
         RENDER_MODE_SCALEY[N], RENDER_MODE_PIXELS[N]);
#define HANDLE_RENDER_MODE(N)         \
  case N: {                           \
    if (!RENDER_MODE_IS_INVALID(N)) { \
      if (state.isRLE)                \
        RENDER(N, true)               \
      else                            \
        RENDER(N, false)              \
    }                                 \
    break;                            \
  }

  // (this creates multiple copies of render(...)'s code)
  switch (config.renderMode) {
    HANDLE_RENDER_MODE(0)
    HANDLE_RENDER_MODE(1)
    HANDLE_RENDER_MODE(2)
    HANDLE_RENDER_MODE(3)
    HANDLE_RENDER_MODE(4)
    HANDLE_RENDER_MODE(5)
    HANDLE_RENDER_MODE(6)
    HANDLE_RENDER_MODE(7)
    HANDLE_RENDER_MODE(8)
    default:
      break;
  }
}
