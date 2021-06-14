#include "Benchmark.h"
#include "GBARemotePlay.h"

int main() {
  LOG("Starting...\n");

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
