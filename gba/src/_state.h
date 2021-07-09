#ifndef STATE_H
#define STATE_H

#include <tonc.h>

#include "Protocol.h"

typedef struct {
  u8 temporalDiffs[TEMPORAL_DIFF_PADDED_SIZE];
  u32 expectedPackets;
  u32 startPixel;
  u32 audioBufferHead;
  u32 audioBufferTail;
  u32 readyAudioChunks;
  bool isRLE;
  bool hasAudio;
  bool isVBlank;
  bool isRunningAudio;
} State;

extern State state;
extern u8 audioBuffer[AUDIO_CHUNKS_PER_BUFFER][AUDIO_PADDED_SIZE];

#endif  // STATE_H
