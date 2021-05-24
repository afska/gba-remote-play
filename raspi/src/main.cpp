#include "SPIMaster.h"

SPIMaster* spiMaster = new SPIMaster();

int main() {
  std::cout << "Starting...\n";

  while (true) {
    uint32_t value = spiMaster->transfer(0x98765432);
  }

  return 0;
}
