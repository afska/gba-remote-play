#ifndef STATE_H
#define STATE_H

#include <tonc.h>

#include "Protocol.h"

typedef struct {
  u8 temporalDiffs[TEMPORAL_DIFF_MAX_PADDED_SIZE];
  u8 audioChunks[AUDIO_PADDED_SIZE];
  u32 expectedPackets;
  u32 startPixel;
  bool isRLE;
  bool hasAudio;
  bool isVBlank;
  bool isAudioReady;

  u32 baseColumn;
  u32 baseLine;
  u32 scaleX;
  u32 scaleY;
  bool scanlines;
} State;

extern State state;

#endif  // STATE_H
