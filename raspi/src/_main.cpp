#include "GBARemotePlay.h"

int main() {
  LOG("Starting...\n");

  auto remotePlay = new GBARemotePlay();

  while (true) {
    remotePlay->run();
  }

  delete remotePlay;

  return 0;
}
