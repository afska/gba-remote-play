#ifndef CONFIG_H
#define CONFIG_H

#include <iostream>
#include <string>

// TODO: MAKE `DEBUG` STATIC, LIKE `BENCHMARK`
#define DEBUG false
// #define BENCHMARK

inline void LOG(std::string STR) {
  std::cout << STR + "\n";
}

inline void DEBULOG(std::string STR) {
  if (DEBUG)
    LOG(STR);
}

#endif  // CONFIG_H
