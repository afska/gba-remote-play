#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <iostream>
#include <string>

#define PROFILE_START() std::chrono::high_resolution_clock::now()
#define PROFILE_END(START)                               \
  std::chrono::duration_cast<std::chrono::milliseconds>( \
      std::chrono::high_resolution_clock::now() - START) \
      .count()

inline void LOG(std::string STR) {
  std::cout << STR + "\n";
}

inline void DEBULOG(std::string STR) {
#ifdef DEBUG
  std::cout << STR + "\n";
#endif
}

#endif  // UTILS_H
