#include "_state.h"
#include "Utils.h"

DATA_IWRAM State state;
DATA_EWRAM u8 audioBuffer[AUDIO_CHUNKS_PER_BUFFER][AUDIO_PADDED_SIZE];
