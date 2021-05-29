#include <tonc.h>

#include <string>
#include "LinkSPI.h"
#include "Protocol.h"
#include "Utils.h"

void onVBlank();
void init();

LinkSPI* linkSPI = new LinkSPI();
u32 frame = 0;
u32 cursor = 0;

inline u32 x() {
  return cursor % RENDER_WIDTH;
}

inline u32 y() {
  return cursor / RENDER_WIDTH;
}

CODE_IWRAM void mainLoop() {
  while (true) {
    u32 receivedPacket = linkSPI->transfer(0x12345678);

    if (receivedPacket == COMMAND_FRAME_START)
      cursor = 0;
    else {
      u16 firstPixel = receivedPacket & 0xffff;
      u16 secondPixel = (receivedPacket >> 16) & 0xffff;
      m3_plot(x(), y(), firstPixel);
      cursor++;
      m3_plot(x(), y(), secondPixel);
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
  REG_DISPCNT = DCNT_MODE3 | DCNT_BG2;

  irq_init(NULL);
  irq_add(II_VBLANK, onVBlank);
}
