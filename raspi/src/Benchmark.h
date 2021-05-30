#include <stdint.h>
#include <iostream>
#include "Config.h"
#include "SPIMaster.h"

#ifdef BENCHMARK

namespace Benchmark {

auto spiMaster = new SPIMaster();
uint32_t goodPackets = 0;
uint32_t badPackets = 0;

inline void main() {
  while (true) {
    uint32_t receivedPacket = spiMaster->transfer(0x98765432);

    if (receivedPacket == 0x12345678)
      goodPackets++;
    else
      badPackets++;

    if (goodPackets + badPackets >= 60000) {
      std::cout << std::to_string(goodPackets) + " vs " +
                       std::to_string(badPackets) + "\n";
      goodPackets = 0;
      badPackets = 0;
    }
  }
}

}  // namespace Benchmark

#endif  // BENCHMARK_H
