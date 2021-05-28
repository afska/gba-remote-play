#include <tonc.h>

#include <string>
#include "LinkSPI.h"
#include "Utils.h"

void onVBlank();
void init();

LinkSPI* linkSPI = new LinkSPI();
u32 frame = 0;
u32 cursor = 0;

CODE_IWRAM void mainLoop() {
  while (true) {
    u32 receivedPacket = linkSPI->transfer(0x12345678);

    if (receivedPacket == 0x98765432)
      cursor = 0;
    else {
      u16 firstPixel = (receivedPacket >> 16) & 0xffff;
      u16 secondPixel = (receivedPacket & 0xffff0000) >> 16;
      m3_plot((cursor % RENDER_WIDTH) * RENDER_SCALE,
              (cursor / RENDER_WIDTH) * RENDER_SCALE, firstPixel);
      cursor++;
      m3_plot((cursor % RENDER_WIDTH) * RENDER_SCALE,
              (cursor / RENDER_WIDTH) * RENDER_SCALE, secondPixel);
      cursor++;
    }
  }
}

int main() {
  init();

  mainLoop();

  return 0;
}

inline void onVBlank() {
  // TODO: REFRESH
}

inline void init() {
  // bitmap mode 3, background 2
  REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

  // enable mosaic
  REG_MOSAIC = MOS_BUILD(1, 1, 0, 0);
  REG_BG2CNT = 1 << 6;

  // interrupts
  irq_init(NULL);
  irq_add(II_VBLANK, onVBlank);
}
