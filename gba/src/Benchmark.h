#include "Config.h"
#include "SPISlave.h"

#ifdef BENCHMARK

#include <tonc.h>
#include <string>

#define BENCHMARK_MIN_SUCCESS 50000
#define BENCHMARK_MAX_ERROR 5

namespace Benchmark {

// inline void log(std::string text) {
//   tte_erase_screen();
//   tte_write("#{P:0,0}");
//   tte_write(text.c_str());
// }

typedef struct {
  u32 frame;
  u32 goodPackets;
  u32 badPackets;
} State;

inline void onVBlank(State& state) {
  state.frame++;
  if (state.frame >= 60) {
    // log(
    //   std::to_string(state.goodPackets) + " vs " +
    //   std::to_string(state.badPackets)
    // );
    m3_plot(20, 80,
            state.goodPackets >= BENCHMARK_MIN_SUCCESS + 5000 ? CLR_GREEN
                                                              : CLR_RED);
    m3_plot(30, 80,
            state.goodPackets >= BENCHMARK_MIN_SUCCESS + 10000 ? CLR_GREEN
                                                               : CLR_RED);
    m3_plot(40, 80,
            state.goodPackets >= BENCHMARK_MIN_SUCCESS + 15000 ? CLR_GREEN
                                                               : CLR_RED);
    m3_plot(50, 80,
            state.goodPackets >= BENCHMARK_MIN_SUCCESS + 20000 ? CLR_GREEN
                                                               : CLR_RED);
    m3_plot(220, 80,
            state.badPackets > BENCHMARK_MAX_ERROR ? CLR_RED : CLR_GREEN);
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
    if (!wasVBlank && isVBlank) {
      onVBlank(state);
      wasVBlank = true;
    } else if (wasVBlank && !isVBlank)
      wasVBlank = false;

    u32 receivedPacket = spiSlave->transfer(0x12345678);

    if (receivedPacket == 0x98765432)
      state.goodPackets++;
    else
      state.badPackets++;
  }
}

inline void init() {
  // REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
  // tte_init_se_default(0, BG_CBB(0) | BG_SBB(31));

  REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;
}

}  // namespace Benchmark

#endif  // BENCHMARK_H
