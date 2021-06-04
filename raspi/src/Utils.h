#ifndef UTILS_H
#define UTILS_H

#include <iostream>
#include <string>

inline void LOG(std::string STR) {
  std::cout << STR + "\n";
}

inline void DEBULOG(std::string STR) {
#ifdef DEBUG
  std::cout << STR + "\n";
#endif
}

#endif  // UTILS_H
