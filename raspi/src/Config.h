#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <fstream>
#include <streambuf>
#include "Utils.h"

class Config {
 public:
  uint32_t spiSlowFrequency = 0;
  uint32_t spiFastFrequency = 0;
  uint32_t spiDelayMicroseconds = 0;
  uint32_t diffThreshold = 0;
  std::string virtualGamepadName = "";

  Config(std::string fileName) {
    std::ifstream file(fileName);
    std::string data((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
    parse(data);

    if (!spiSlowFrequency || !spiFastFrequency || !spiDelayMicroseconds ||
        !diffThreshold || virtualGamepadName == "") {
      std::cout << "Error (Config): invalid configuration, check " + fileName +
                       "\n";
      exit(1);
    }
  }

 private:
  void parse(std::string data) {
    auto lines = split(data, "\n");

    for (auto& line : lines) {
      auto parts = split(line, "=");
      if (parts.size() != 2)
        continue;

      auto key = parts.at(0);
      auto value = parts.at(1);

      if (key == "SPI_SLOW_FREQUENCY")
        spiSlowFrequency = std::stoi(value);
      else if (key == "SPI_FAST_FREQUENCY")
        spiFastFrequency = std::stoi(value);
      else if (key == "SPI_DELAY_MICROSECONDS")
        spiDelayMicroseconds = std::stoi(value);
      else if (key == "DIFF_THRESHOLD")
        diffThreshold = std::stoi(value);
      else if (key == "VIRTUAL_GAMEPAD_NAME")
        virtualGamepadName = value;
    }
  }
};

#endif  // CONFIG_H
