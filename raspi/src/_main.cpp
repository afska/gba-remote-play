#include <iostream>
#include "Benchmark.h"
#include "GBARemotePlay.h"

int main() {
  std::cout << "Starting...\n\n";

#ifdef BENCHMARK
  Benchmark::main();
#endif

  auto remotePlay = new GBARemotePlay();

  while (true) {
    remotePlay->run();
  }

  delete remotePlay;

  return 0;
}
