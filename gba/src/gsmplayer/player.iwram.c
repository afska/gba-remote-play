#include "player.h"

#include "GSMPlayer.h"
#include "PlaybackState.h"

Playback PlaybackState;

PLAYER_DEFINE(REG_DMA1CNT,
              REG_DMA1SAD,
              REG_DMA1DAD,
              FIFO_ADDR_A,
              CHANNEL_A_MUTE,
              CHANNEL_A_UNMUTE);

inline void player_init() {
  PLAYER_TURN_ON_SOUND();
  PLAYER_INIT(REG_TM0CNT_L, REG_TM0CNT_H);
}

inline void player_play(const char* name) {
  PLAYER_PLAY(name);
  PlaybackState.hasFinished = false;
  PlaybackState.isLooping = false;
}

inline void player_loop(const char* name) {
  player_play(name);
  PlaybackState.isLooping = true;
}

inline void player_seek(unsigned int msecs) {
  // (cursor must be a multiple of AUDIO_CHUNK_SIZE)
  // cursor = src_pos - src
  // msecs = cursor * msecsPerSample
  // msecsPerSample = AS_MSECS / FRACUMUL_PRECISION ~= 0.267
  // => msecs = cursor * 0.267
  // => cursor = msecs / 0.267 = msecs * 3.7453
  // => cursor = msecs * (3 + 0.7453)

  unsigned int cursor = msecs * 3 + fracumul(msecs, AS_CURSOR);
  cursor = (cursor / AUDIO_CHUNK_SIZE) * AUDIO_CHUNK_SIZE;
  src_pos = src + cursor;
}

inline void player_stop() {
  PLAYER_STOP();
  PlaybackState.hasFinished = false;
  PlaybackState.isLooping = false;
}

inline bool player_isPlaying() {
  return src_pos != NULL;
}

inline void player_run() {
  PLAYER_VBLANK();
  PLAYER_RUN({}, {
    if (PlaybackState.isLooping)
      player_seek(0);
    else {
      player_stop();
      PlaybackState.hasFinished = true;
    }
  });
}
