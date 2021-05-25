#include "GBARemotePlay.h"

int main() {
  std::cout << "Starting...\n";

  auto remotePlay = new GBARemotePlay();

  while (true) {
    remotePlay->run();
  }

  delete remotePlay;

  return 0;
}
