#include <tonc.h>

#include <string>
#include "LinkSPI.h"

void onVBlank();
void init();
void log(std::string text);

LinkSPI* linkSPI = new LinkSPI();
u32 frame = 0;
u32 goodPackets = 0;
u32 badPackets = 0;

int main() {
  init();

  log("Waiting...");

  while (true) {
    u32 receivedPacket = linkSPI->transfer(0x12345678);

    if (receivedPacket == 0x98765432)
      goodPackets++;
    else
      badPackets++;
  }

  return 0;
}

inline void onVBlank() {
  frame++;
  if (frame >= 60) {
    log(std::to_string(goodPackets) + " vs " + std::to_string(badPackets));
    frame = 0;
    goodPackets = 0;
    badPackets = 0;
  }
}

inline void init() {
  REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
  tte_init_se_default(0, BG_CBB(0) | BG_SBB(31));

  irq_init(NULL);
  irq_add(II_VBLANK, onVBlank);
}

inline void log(std::string text) {
  tte_erase_screen();
  tte_write("#{P:0,0}");
  tte_write(text.c_str());
}
