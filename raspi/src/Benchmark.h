#include <stdint.h>
#include "BuildConfig.h"
#include "Protocol.h"
#include "SPIMaster.h"
#include "Utils.h"

#ifdef BENCHMARK

namespace Benchmark {

auto spiMaster = new SPIMaster(SPI_MODE,
                               SPI_SLOW_FREQUENCY,
                               SPI_FAST_FREQUENCY,
                               SPI_DELAY_MICROSECONDS);
uint32_t goodPackets = 0;
uint32_t badPackets = 0;

inline void main() {
  while (true) {
    uint32_t receivedPacket = spiMaster->exchange(0x98765432);

    if (receivedPacket == 0x12345678)
      goodPackets++;
    else
      badPackets++;

    if (goodPackets + badPackets >= 60000) {
      LOG(std::to_string(goodPackets) + " vs " + std::to_string(badPackets));
      goodPackets = 0;
      badPackets = 0;
    }
  }
}

}  // namespace Benchmark

#endif  // BENCHMARK_H
