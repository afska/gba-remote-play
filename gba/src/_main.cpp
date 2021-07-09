#include <tonc.h>

#include "Benchmark.h"
#include "BuildConfig.h"
#include "Palette.h"
#include "Protocol.h"
#include "SPISlave.h"
#include "Utils.h"
#include "_state.h"
#include "gbajam/Demo.h"

extern "C" {
#include "gsmplayer/player.h"
}

#define TRY(ACTION) \
  if (!(ACTION))    \
    goto reset;

// -----
// STATE
// -----

SPISlave* spiSlave = new SPISlave();
DATA_EWRAM u8 compressedPixels[TOTAL_PIXELS];

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
bool sendKeysAndReceiveMetadata();
bool receiveAudio();
bool receivePixels();
void render(bool withRLE);
bool isNewVBlank();
void driveAudio();
u32 transfer(u32 packetToSend);
bool sync(u32 command);
u32 x(u32 cursor);
u32 y(u32 cursor);

// -----------
// DEFINITIONS
// -----------

#ifndef BENCHMARK
int main() {
  Demo::run();

  init();

  for (u32 i = 0; i < TOTAL_SCREEN_PIXELS; i++)
    m4Draw(i, 0);

  mainLoop();

  return 0;
}
#endif

CODE_IWRAM void ON_VBLANK() {
  player_onVBlank();
  REG_IF = IRQ_VBLANK;
}

inline void init() {
  enableMode4AndBackground2();
  overclockEWRAM();
  enableMosaic(DRAW_SCALE_X, DRAW_SCANLINES ? 1 : DRAW_SCALE_Y);
  dma3_cpy(pal_bg_mem, MAIN_PALETTE, sizeof(COLOR) * PALETTE_COLORS);
  player_init();

  REG_ISR_MAIN = ON_VBLANK;  // Tell the GBA where my ISR is
  REG_DISPSTAT |= 0b1000;    // Tell the display to fire VBlank interrupts
  REG_IE |= IRQ_VBLANK;      // Tell the GBA to catch VBlank interrupts
  REG_IME = 1;               // Tell the GBA to enable interrupts;
}

CODE_IWRAM void mainLoop() {
  state.isVBlank = false;

reset:
  state.isAudioStart = false;
  state.audioCursor = 0;
  state.audioSize = 0;

  for (u32 i = 0; i < AUDIO_INITIAL_CHUNKS; i++)
    TRY(receiveAudio())

  while (true) {
    TRY(sync(CMD_FRAME_START))
    TRY(sendKeysAndReceiveMetadata())
    TRY(sync(CMD_AUDIO))
    TRY(receiveAudio())
    TRY(sync(CMD_PIXELS))
    TRY(receivePixels())
    TRY(sync(CMD_FRAME_END))

    // (gcc optimizes this better than `render(state.isRLE)`)
    if (state.isRLE)
      render(true);
    else
      render(false);
  }
}

inline bool sendKeysAndReceiveMetadata() {
  u16 keys = pressedKeys();
  u32 metadata = spiSlave->transfer(keys);
  if (spiSlave->transfer(metadata) != keys)
    return false;

  state.expectedPackets = (metadata >> PACKS_BIT_OFFSET) & PACKS_BIT_MASK;
  state.startPixel = metadata & START_BIT_MASK;
  state.isRLE = (metadata & COMPR_BIT_MASK) != 0;
  state.isAudioStart = (metadata & AUDIO_BIT_MASK) != 0;

  u32 diffsStart = (state.startPixel / 8) / PACKET_SIZE;
  for (u32 i = diffsStart; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++)
    ((u32*)state.temporalDiffs)[i] = transfer(i);

  return true;
}

inline bool receiveAudio() {
  if (state.audioCursor + AUDIO_SIZE_PACKETS > AUDIO_BUFFER_SIZE)
    state.audioCursor = 0;

  for (u32 i = 0; i < AUDIO_SIZE_PACKETS; i++)
    audioBuffer[state.audioCursor + i] = transfer(i);

  state.audioCursor += AUDIO_SIZE_PACKETS;
  state.audioSize += AUDIO_SIZE_PACKETS;
  player_updateMediaSize(AUDIO_SIZE_PACKETS * PACKET_SIZE);

  if (state.isAudioStart) {
    player_play((const unsigned char*)audioBuffer,
                state.audioSize * PACKET_SIZE);
  }

  return true;
}

inline bool receivePixels() {
  for (u32 i = 0; i < state.expectedPackets; i++)
    ((u32*)compressedPixels)[i] = transfer(i);

  return true;
}

inline void render(bool withRLE) {
  u32 cursor = state.startPixel;
  u32 rleRepeats = compressedPixels[0];
  u32 decompressedBytes = withRLE;

#define DRIVE_AUDIO_IF_NEEDED()         \
  if (withRLE) {                        \
    if (isNewVBlank())                  \
      driveAudio();                     \
  } else {                              \
    if (!(cursor % 8) && isNewVBlank()) \
      driveAudio();                     \
  }
#define DRAW_PIXEL(PIXEL) m4Draw(y(cursor) * DRAW_WIDTH + x(cursor), PIXEL);
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
#define DRAW_BATCH(TIMES)                         \
  u32 target = min(cursor + TIMES, TOTAL_PIXELS); \
  while (cursor < target) {                       \
    DRIVE_AUDIO_IF_NEEDED()                       \
    DRAW_NEXT()                                   \
  }

  while (cursor < TOTAL_PIXELS) {
    DRIVE_AUDIO_IF_NEEDED()
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

inline bool isNewVBlank() {
  if (!state.isVBlank && IS_VBLANK) {
    state.isVBlank = true;
    return true;
  } else if (state.isVBlank && !IS_VBLANK)
    state.isVBlank = false;

  return false;
}

CODE_IWRAM void driveAudio() {
  player_run();
}

inline u32 transfer(u32 packetToSend) {
  u32 receivedPacket = spiSlave->transfer(packetToSend);

  if (isNewVBlank())
    driveAudio();

  return receivedPacket;
}

inline bool sync(u32 command) {
  u32 local = command + CMD_GBA_OFFSET;
  u32 remote = command + CMD_RPI_OFFSET;
  bool wasVBlank = IS_VBLANK;

  while (true) {
    bool isOnSync = spiSlave->transfer(local) == remote;

    if (isNewVBlank())
      driveAudio();

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
