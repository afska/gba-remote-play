#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <fstream>
#include <streambuf>
#include "SPIMaster.h"
#include "Utils.h"

class Config {
 public:
  SPITiming spiNormalTiming;
  SPITiming spiOverclockedTiming;
  std::string virtualGamepadName = "";

  Config(std::string fileName) {
    std::ifstream file(fileName);
    std::string data((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());

    spiNormalTiming.reset();
    spiOverclockedTiming.reset();
    parse(data);

    if (!spiNormalTiming.slowFrequency || !spiNormalTiming.fastFrequency ||
        !spiNormalTiming.delayMicroseconds ||
        !spiOverclockedTiming.slowFrequency ||
        !spiOverclockedTiming.fastFrequency ||
        !spiOverclockedTiming.delayMicroseconds || virtualGamepadName == "") {
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
        spiNormalTiming.slowFrequency = std::stoi(value);
      else if (key == "SPI_FAST_FREQUENCY")
        spiNormalTiming.fastFrequency = std::stoi(value);
      else if (key == "SPI_DELAY_MICROSECONDS")
        spiNormalTiming.delayMicroseconds = std::stoi(value);
      else if (key == "SPI_OVERCLOCKED_SLOW_FREQUENCY")
        spiOverclockedTiming.slowFrequency = std::stoi(value);
      else if (key == "SPI_OVERCLOCKED_FAST_FREQUENCY")
        spiOverclockedTiming.fastFrequency = std::stoi(value);
      else if (key == "SPI_OVERCLOCKED_DELAY_MICROSECONDS")
        spiOverclockedTiming.delayMicroseconds = std::stoi(value);
      else if (key == "VIRTUAL_GAMEPAD_NAME")
        virtualGamepadName = value;
    }
  }
};

#endif  // CONFIG_H
