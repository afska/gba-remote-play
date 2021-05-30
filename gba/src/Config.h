#ifndef CONFIG_H
#define CONFIG_H

#define CODE_IWRAM __attribute__((section(".iwram"), target("arm")))
#define BENCHMARK false

#endif  // CONFIG_H
