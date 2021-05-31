#ifndef CONFIG_H
#define CONFIG_H

#define CODE_IWRAM __attribute__((section(".iwram"), target("arm")))
#define IS_VBLANK (REG_VCOUNT >= 160)
// #define BENCHMARK

#endif  // CONFIG_H
