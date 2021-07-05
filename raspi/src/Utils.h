#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

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

#define ONE_SECOND 1000

inline int getDistanceSquared(int r1, int g1, int b1, int color2) {
  int r2 = (color2 >> 0) & 0xff;
  int g2 = (color2 >> 8) & 0xff;
  int b2 = (color2 >> 16) & 0xff;

  int diffR = r1 - r2;
  int diffG = g1 - g2;
  int diffB = b1 - b2;

  return diffR * diffR + diffG * diffG + diffB * diffB;
}

inline std::vector<std::string> split(std::string str, std::string delimiter) {
  std::vector<std::string> output;

  auto start = 0U;
  auto end = str.find(delimiter);
  while (end != std::string::npos) {
    output.push_back(str.substr(start, end - start));
    start = end + delimiter.length();
    end = str.find(delimiter, start);
  }

  output.push_back(str.substr(start, end));

  return output;
}

#endif  // UTILS_H
