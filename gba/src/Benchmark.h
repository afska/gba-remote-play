#include "BuildConfig.h"

#ifdef BENCHMARK

#include "SPISlave.h"
#include "Utils.h"

namespace Benchmark {

typedef struct {
  u32 frame;
  u32 goodPackets;
  u32 badPackets;
} State;

inline void onVBlank(State& state) {
  state.frame++;
  if (state.frame >= 60) {
    m3_plot(20, 80, state.goodPackets >= 40000 ? CLR_GREEN : CLR_RED);
    m3_plot(30, 80, state.goodPackets >= 45000 ? CLR_GREEN : CLR_RED);
    m3_plot(40, 80, state.goodPackets >= 50000 ? CLR_GREEN : CLR_RED);
    m3_plot(50, 80, state.goodPackets >= 55000 ? CLR_GREEN : CLR_RED);
    m3_plot(60, 80, state.goodPackets >= 60000 ? CLR_GREEN : CLR_RED);
    m3_plot(70, 80, state.goodPackets >= 65000 ? CLR_GREEN : CLR_RED);
    m3_plot(80, 80, state.goodPackets >= 67500 ? CLR_GREEN : CLR_RED);
    m3_plot(90, 80, state.goodPackets >= 70000 ? CLR_GREEN : CLR_RED);
    m3_plot(100, 80, state.goodPackets >= 75000 ? CLR_GREEN : CLR_RED);
    m3_plot(110, 80, state.goodPackets >= 80000 ? CLR_GREEN : CLR_RED);
    m3_plot(220, 80, state.badPackets > 0 ? CLR_RED : CLR_GREEN);
    state.frame = 0;
    state.goodPackets = 0;
    state.badPackets = 0;
  }
}

CODE_IWRAM void mainLoop() {
  bool wasVBlank = false;
  State state;
  state.frame = 0;
  state.goodPackets = 0;
  state.badPackets = 0;

  while (true) {
    bool isVBlank = IS_VBLANK;
    u32 packetToSend = 0x12345678;
    if (!wasVBlank && isVBlank) {
      onVBlank(state);
      wasVBlank = true;
      packetToSend = 0x123456bb;
    } else if (wasVBlank && !isVBlank)
      wasVBlank = false;

    u32 receivedPacket = spiSlave->transfer(packetToSend);

    if (receivedPacket == 0x98765432)
      state.goodPackets++;
    else
      state.badPackets++;
  }
}

inline void init() {
  REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
}

}  // namespace Benchmark

#endif  // BENCHMARK_H
