#ifndef UTILS_H
#define UTILS_H

#define CODE_IWRAM __attribute__((section(".iwram"), target("arm")))
#define DATA_EWRAM __attribute__((section(".ewram")))
#define GBA_MAX_COLORS 32768
#define IS_VBLANK (REG_VCOUNT >= 160)
#define ENABLE_MODE4_AND_BG2() REG_DISPCNT = DCNT_MODE4 | DCNT_BG2
#define OVERCLOCK_IWRAM() *((u32*)0x4000800) = (0x0E << 24) | (1 << 5)

inline u8 m4Get(u32 cursor) {
  u16* dst = &vid_page[cursor >> 1];
  return cursor & 1 ? (*dst >> 8) & 0xff : *dst & 0xff;
}

inline void m4Draw(u32 cursor, u8 colorIndex) {
  u16* dst = &vid_page[cursor >> 1];
  *dst = cursor & 1 ? (*dst & 0xFF) | (colorIndex << 8)
                    : (*dst & ~0xFF) | colorIndex;
}

#endif  // UTILS_H
