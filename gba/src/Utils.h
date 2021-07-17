#ifndef UTILS_H
#define UTILS_H

#include <tonc.h>

#define CODE_IWRAM __attribute__((section(".iwram"), target("arm")))
#define DATA_IWRAM __attribute__((section(".iwram")))
#define DATA_EWRAM __attribute__((section(".ewram")))
#define BIT_SET_HIGH(REG, BIT) (REG) |= 1 << (BIT)
#define BIT_SET_LOW(REG, BIT) (REG) &= ~(1 << (BIT))
#define BIT_IS_HIGH(REG, BIT) ((REG) & (1 << (BIT)))
#define IS_VBLANK (REG_DISPSTAT & 1)

inline void enableMode4AndBackground2() {
  REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;
}

inline void overclockEWRAM() {
  *((u32*)0x4000800) = (0x0E << 24) | (1 << 5);
}

inline void setMosaic(u16 scaleX, u16 scaleY) {
  u16 x = scaleX - 1;
  u16 y = scaleY - 1;
  REG_MOSAIC = MOS_BUILD(x, y, 0, 0);
  REG_BG2CNT = 1 << 6;
}

inline void m4Draw(u32 cursor, u8 colorIndex) {
  u16* dst = &vid_mem_front[cursor >> 1];
  *dst = cursor & 1 ? (*dst & 0xFF) | (colorIndex << 8)
                    : (*dst & ~0xFF) | colorIndex;
}

inline u16 pressedKeys() {
  return ~REG_KEYS & KEY_ANY;
}

inline bool needsRestart() {
  u16 keys = pressedKeys();

  return (keys & KEY_A) && (keys & KEY_B) && (keys & KEY_L) && (keys & KEY_R);
}

#endif  // UTILS_H
