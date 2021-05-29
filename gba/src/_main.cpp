#include <tonc.h>

#include "LinkSPI.h"
#include "Protocol.h"
#include "Utils.h"

void onVBlank();
void init();

LinkSPI* linkSPI = new LinkSPI();
u32 frame = 0;
u32 cursor = 0;
bool isReady = false;
COLOR palette[PALETTE_COLORS];

inline u32 x() {
  return cursor % RENDER_WIDTH;
}

inline u32 y() {
  return cursor / RENDER_WIDTH;
}

inline void sync(u32 local, u32 remote) {
  while (linkSPI->transfer(local) != remote)
    ;
}

CODE_IWRAM void mainLoop() {
  while (true) {
    if (isReady)
      VBlankIntrWait();

    sync(CMD_FRAME_START_GBA, CMD_FRAME_START_RPI);

    sync(CMD_PIXELS_START_GBA, CMD_PIXELS_START_RPI);
    cursor = 0;
    u32 packet = 0;
    while ((packet = linkSPI->transfer(0)) != CMD_PALETTE_START_GBA) {
      if (x() >= RENDER_WIDTH || y() >= RENDER_HEIGHT)
        break;

      ((u32*)vid_page)[(y() * RENDER_WIDTH + x()) / PIXELS_PER_PACKET] = packet;
      cursor += PIXELS_PER_PACKET;
    }

    sync(CMD_PALETTE_START_GBA, CMD_PALETTE_START_RPI);
    for (u32 i = 0; i < PALETTE_COLORS; i += COLORS_PER_PACKET) {
      u32 packet = linkSPI->transfer(0);
      palette[i] = packet & 0xffff;
      palette[i + 1] = (packet >> 16) & 0xffff;
    }

    isReady = true;
    sync(CMD_FRAME_END_GBA, CMD_FRAME_END_RPI);
  }
}

int main() {
  init();

  mainLoop();

  return 0;
}

inline void onVBlank() {
  if (isReady) {
    tonccpy(pal_bg_mem, &palette, sizeof(COLOR) * PALETTE_COLORS);
    vid_flip();
  }
  isReady = false;
}

inline void init() {
  REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;

  irq_init(NULL);
  irq_add(II_VBLANK, onVBlank);
}
