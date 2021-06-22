#ifndef UTILS_H
#define UTILS_H

#include <tonc.h>

#define CODE_IWRAM __attribute__((section(".iwram"), target("arm")))
#define DATA_EWRAM __attribute__((section(".ewram")))
#define IS_VBLANK (REG_VCOUNT >= 160)

inline void enableMode4AndBackground2() {
  REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;
}

inline void overclockEWRAM() {
  *((u32*)0x4000800) = (0x0E << 24) | (1 << 5);
}

inline void enable2xMosaic() {
  REG_MOSAIC = MOS_BUILD(1, 1, 0, 0);
  REG_BG2CNT = 1 << 6;
}

inline u16 pressedKeys() {
  return ~REG_KEYS & KEY_ANY;
}

typedef struct Memcpy32HookBlock {
  u32 data[8];
} Memcpy32HookBlock;

template <typename F>
CODE_IWRAM void memcpy32Hook(void* dst, const void* src, uint wdcount, F hook) {
  u32 blkN = wdcount / 8, wdN = wdcount & 7;
  u32 *dstw = (u32*)dst, *srcw = (u32*)src;
  if (blkN) {
    // (8-word copies)
    Memcpy32HookBlock *dst2 = (Memcpy32HookBlock*)dst,
                      *src2 = (Memcpy32HookBlock*)src;
    while (blkN--) {
      *dst2++ = *src2++;
      hook();
    }
    dstw = (u32*)dst2;
    srcw = (u32*)src2;
  }

  // (residual words)
  while (wdN--) {
    *dstw++ = *srcw++;
    hook();
  }
}

#endif  // UTILS_H
