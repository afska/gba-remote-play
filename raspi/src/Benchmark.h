#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdint.h>
#include "BuildConfig.h"
#include "Config.h"
#include "Protocol.h"
#include "SPIMaster.h"
#include "Utils.h"

namespace Benchmark {

inline void main(uint32_t renderMode) {
  auto config = new Config(CONFIG_FILENAME);
  auto spiMaster = new SPIMaster(
      SPI_MODE,
      (SPITiming){
          .slowFrequency = renderMode == RENDER_MODE_BENCHMARK_2
                               ? config->spiNormalTiming.fastFrequency
                               : config->spiNormalTiming.slowFrequency,
          .fastFrequency = config->spiNormalTiming.fastFrequency,
          .delayMicroseconds = config->spiNormalTiming.delayMicroseconds},
      (SPITiming){
          .slowFrequency = 0, .fastFrequency = 0, .delayMicroseconds = 0});

  uint32_t goodPackets = 0;
  uint32_t badPackets = 0;
  uint32_t vblanks = 0;

  while (true) {
    uint32_t receivedPacket = spiMaster->exchange(0x98765432);

    if (receivedPacket == 0x123456bb) {
      goodPackets++;
      vblanks++;
    } else if (receivedPacket == 0x12345678)
      goodPackets++;
    else if (receivedPacket != 0xffffffff)
      badPackets++;

    if (vblanks >= 60) {
      LOG(std::to_string(goodPackets) + " vs " + std::to_string(badPackets));
      goodPackets = 0;
      badPackets = 0;
      vblanks = 0;
    }
  }
}

}  // namespace Benchmark

#endif  // BENCHMARK_H
