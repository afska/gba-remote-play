#ifndef UTILS_H
#define UTILS_H

#define CODE_IWRAM __attribute__((section(".iwram"), target("arm")))
#define DATA_EWRAM __attribute__((section(".ewram")))
#define IS_VBLANK (REG_VCOUNT >= 160)
#define GBA_MAX_COLORS 32768
#define ENABLE_MODE4_AND_BG2() REG_DISPCNT = DCNT_MODE4 | DCNT_BG2
#define OVERCLOCK_IWRAM() *((u32*)0x4000800) = (0x0E << 24) | (1 << 5)

#endif  // UTILS_H
