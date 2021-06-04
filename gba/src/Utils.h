#ifndef UTILS_H
#define UTILS_H

#define CODE_IWRAM __attribute__((section(".iwram"), target("arm")))
#define DATA_EWRAM __attribute__((section(".ewram")))
#define GBA_MAX_COLORS 32768
#define IS_VBLANK (REG_VCOUNT >= 160)

inline void enableMode4AndBackground2() {
  REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;
}

inline void overclockIWRAM() {
  *((u32*)0x4000800) = (0x0E << 24) | (1 << 5);
}

inline void enable2xMosaic() {
  REG_MOSAIC = MOS_BUILD(1, 1, 0, 0);
  REG_BG2CNT = 1 << 6;
}

inline u8 m4Get(u32 cursor) {
  u16* dst = &vid_page[cursor >> 1];
  return cursor & 1 ? (*dst >> 8) & 0xff : *dst & 0xff;
}

inline u8 m4GetXYFrom(u16* buffer, u32 x, u32 y) {
  u16* dst = &buffer[(y * M4_WIDTH + x) >> 1];
  return x & 1 ? (*dst >> 8) & 0xff : *dst & 0xff;
}

#endif  // UTILS_H
