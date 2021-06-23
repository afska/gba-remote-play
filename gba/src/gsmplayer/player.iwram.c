#include "player.h"

#include "GSMPlayer.h"

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

inline void player_play(char* chunk) {
  PLAYER_PLAY(chunk);
}

inline void player_stop() {
  PLAYER_STOP();
}

inline void player_run() {
  PLAYER_VBLANK();
  PLAYER_RUN({}, { player_stop(); });
}
