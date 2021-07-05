#ifndef STATE_H
#define STATE_H

#include <tonc.h>

#include "Protocol.h"

typedef struct {
  u8 temporalDiffs[TEMPORAL_DIFF_PADDED_SIZE];
  u8 audioChunks[AUDIO_PADDED_SIZE];
  u32 expectedPackets;
  u32 startPixel;
  bool hasAudio;
  bool isVBlank;
  bool isAudioReady;
} State;

extern State state;

#endif  // STATE_H
