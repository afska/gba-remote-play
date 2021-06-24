#ifndef GSM_PLAYER_H
#define GSM_PLAYER_H

#include <gba_dma.h>
#include <gba_input.h>
#include <gba_interrupt.h>
#include <gba_sound.h>
#include <gba_systemcalls.h>
#include <gba_timers.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>  // for memset
#include "core/gsm.h"
#include "core/private.h" /* for sizeof(struct gsm_state) */
#include "gbfs/gbfs.h"

#define TIMER_16MHZ 0
#define FIFO_ADDR_A 0x040000a0
#define FIFO_ADDR_B 0x040000A4
#define CHANNEL_A_MUTE 0b1111110011111111
#define CHANNEL_A_UNMUTE 0b0000001100000000
#define CHANNEL_B_MUTE 0b1100111111111111
#define CHANNEL_B_UNMUTE 0b0011000000000000
#define AUDIO_CHUNK_SIZE 66  // 2 gsm frames
#define DATA_IWRAM __attribute__((section(".iwram")))

#define PLAYER_DEFINE(DMA_CNT, DMA_SAD, DMA_DAD, FIFO_ADDRESS, MUTE, UNMUTE)   \
  uint32_t fracumul(uint32_t x, uint32_t frac) __attribute__((long_call));     \
                                                                               \
  static const GBFS_FILE* fs;                                                  \
  DATA_IWRAM static signed char double_buffers[2][608]                         \
      __attribute__((aligned(4)));                                             \
  DATA_IWRAM unsigned int cur_buffer = 0;                                      \
  DATA_IWRAM const unsigned char* src_pos = NULL;                              \
  DATA_IWRAM const unsigned char* src_end = NULL;                              \
                                                                               \
  DATA_IWRAM struct gsm_state decoder;                                         \
  DATA_IWRAM signed short out_samples[160];                                    \
  DATA_IWRAM unsigned int decode_pos = 160;                                    \
  DATA_IWRAM signed char* dst_pos;                                             \
  DATA_IWRAM int last_sample = 0;                                              \
  DATA_IWRAM int i;                                                            \
                                                                               \
  inline void gsm_init(gsm r) {                                                \
    memset((char*)r, 0, sizeof(*r));                                           \
    r->nrp = 40;                                                               \
  }                                                                            \
                                                                               \
  inline void dsound_switch_buffers(const void* data) {                        \
    DMA_CNT = 0;                                                               \
                                                                               \
    DMA_SAD = (intptr_t)data;                                                  \
    DMA_DAD = (intptr_t)FIFO_ADDRESS;                                          \
    DMA_CNT = DMA_DST_FIXED | DMA_SRC_INC | DMA_REPEAT | DMA32 | DMA_SPECIAL | \
              DMA_ENABLE | 1;                                                  \
  }                                                                            \
                                                                               \
  inline void mute() { DSOUNDCTRL = DSOUNDCTRL & MUTE; }                       \
                                                                               \
  inline void unmute() { DSOUNDCTRL = DSOUNDCTRL | UNMUTE; }

#define PLAYER_TURN_ON_SOUND() \
  SETSNDRES(1);                \
  SNDSTAT = SNDSTAT_ENABLE;    \
  DSOUNDCTRL = 0b1111101100001110;

#define PLAYER_INIT(TM_CNT_L, TM_CNT_H)        \
  /* TMxCNT_L is count; TMxCNT_H is control */ \
  TM_CNT_H = 0;                                \
  TM_CNT_L = 0x10000 - (924 / 2);              \
  TM_CNT_H = TIMER_16MHZ | TIMER_START;        \
                                               \
  gsm_init(&decoder);                          \
  mute();

#define PLAYER_PLAY(SRC) \
  src_pos = SRC;         \
  src_end = SRC + AUDIO_CHUNK_SIZE;

#define PLAYER_STOP() \
  src_pos = NULL;     \
  src_end = NULL;     \
  mute();

#define PLAYER_VBLANK()                              \
  dsound_switch_buffers(double_buffers[cur_buffer]); \
                                                     \
  if (src_pos != NULL)                               \
    unmute();                                        \
                                                     \
  cur_buffer = !cur_buffer;

#define PLAYER_RUN(ON_STEP, ON_STOP)                  \
  dst_pos = double_buffers[cur_buffer];               \
                                                      \
  if (src_pos < src_end) {                            \
    for (i = 304 / 4; i > 0; i--) {                   \
      int cur_sample;                                 \
      if (decode_pos >= 160) {                        \
        if (src_pos < src_end)                        \
          gsm_decode(&decoder, src_pos, out_samples); \
        src_pos += sizeof(gsm_frame);                 \
        decode_pos = 0;                               \
        ON_STEP;                                      \
      }                                               \
                                                      \
      /* 2:1 linear interpolation */                  \
      cur_sample = out_samples[decode_pos++];         \
      *dst_pos++ = (last_sample + cur_sample) >> 9;   \
      *dst_pos++ = cur_sample >> 8;                   \
      last_sample = cur_sample;                       \
                                                      \
      cur_sample = out_samples[decode_pos++];         \
      *dst_pos++ = (last_sample + cur_sample) >> 9;   \
      *dst_pos++ = cur_sample >> 8;                   \
      last_sample = cur_sample;                       \
                                                      \
      cur_sample = out_samples[decode_pos++];         \
      *dst_pos++ = (last_sample + cur_sample) >> 9;   \
      *dst_pos++ = cur_sample >> 8;                   \
      last_sample = cur_sample;                       \
                                                      \
      cur_sample = out_samples[decode_pos++];         \
      *dst_pos++ = (last_sample + cur_sample) >> 9;   \
      *dst_pos++ = cur_sample >> 8;                   \
      last_sample = cur_sample;                       \
    }                                                 \
  } else if (src_pos != NULL) {                       \
    ON_STOP;                                          \
  }

#endif  // GSM_PLAYER_H
