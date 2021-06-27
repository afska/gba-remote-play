#ifndef UTILS_H
#define UTILS_H

#include <tonc.h>

#define CODE_IWRAM __attribute__((section(".iwram"), target("arm")))
#define DATA_EWRAM __attribute__((section(".ewram")))
#define IS_VBLANK (REG_DISPSTAT & 1)

inline void enableMode4AndBackground2() {
  REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;
}

inline void overclockEWRAM() {
  *((u32*)0x4000800) = (0x0E << 24) | (1 << 5);
}

inline void enableMosaic(u16 scaleX, u16 scaleY) {
  u16 x = scaleX - 1;
  u16 y = scaleY - 1;
  REG_MOSAIC = MOS_BUILD(x, y, 0, 0);
  REG_BG2CNT = 1 << 6;
}

inline u16 pressedKeys() {
  return ~REG_KEYS & KEY_ANY;
}

#endif  // UTILS_H
