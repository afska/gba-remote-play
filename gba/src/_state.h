#ifndef STATE_H
#define STATE_H

#include <tonc.h>

#include "Protocol.h"

#define AUDIO_BUFFER_SIZE 50000  // in 32-bit words
#define AUDIO_INITIAL_CHUNKS 3

typedef struct {
  u8 temporalDiffs[TEMPORAL_DIFF_PADDED_SIZE];
  u32 expectedPackets;
  u32 startPixel;
  u32 audioCursor;
  u32 audioSize;
  bool isRLE;
  bool hasAudio;
  bool isVBlank;
  bool isRunningAudio;
} State;

extern State state;
extern u32 audioBuffer[AUDIO_BUFFER_SIZE];

#endif  // STATE_H
