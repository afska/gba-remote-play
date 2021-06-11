#ifndef UTILS_H
#define UTILS_H

#include <tonc.h>

#define CODE_IWRAM __attribute__((section(".iwram"), target("arm")))
#define DATA_EWRAM __attribute__((section(".ewram")))
#define GBA_MAX_COLORS 32768
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

#endif  // UTILS_H
