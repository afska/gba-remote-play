#include <tonc.h>

#include "LinkSPI.h"
#include "Protocol.h"
#include "Utils.h"

void init();
void mainLoop();
void onVBlank();
bool sync(u32 local, u32 remote);
u32 x();
u32 y();

LinkSPI* linkSPI = new LinkSPI();

u32 frame = 0;
u32 cursor = 0;
bool isReady = false;
u32 blindFrames = 0;

int main() {
  init();

  mainLoop();

  return 0;
}

inline void init() {
  REG_DISPCNT = DCNT_MODE4 | DCNT_BG2;

  irq_init(NULL);
  irq_add(II_VBLANK, (fnptr)onVBlank);
}

CODE_IWRAM void mainLoop() {
  while (true) {
  reset:

    linkSPI->transfer(CMD_RESET);

    if (isReady)
      VBlankIntrWait();

    if (!sync(CMD_FRAME_START_GBA, CMD_FRAME_START_RPI))
      goto reset;

    for (u32 i = 0; i < PALETTE_COLORS; i += COLORS_PER_PACKET) {
      u32 packet = linkSPI->transfer(0);
      pal_obj_mem[i] = packet & 0xffff;
      pal_obj_mem[i + 1] = (packet >> 16) & 0xffff;
    }

    if (!sync(CMD_PIXELS_START_GBA, CMD_PIXELS_START_RPI))
      goto reset;

    cursor = 0;
    u32 packet = 0;
    while ((packet = linkSPI->transfer(0)) != CMD_FRAME_END_RPI) {
      if (x() >= RENDER_WIDTH || y() >= RENDER_HEIGHT)
        break;

      ((u32*)vid_page)[(y() * RENDER_WIDTH + x()) / PIXELS_PER_PACKET] = packet;
      cursor += PIXELS_PER_PACKET;
    }

    isReady = true;
    sync(CMD_FRAME_END_GBA, CMD_FRAME_END_RPI);
  }
}

inline void onVBlank() {
  if (isReady) {
    blindFrames = 0;
    tonccpy(pal_bg_mem, pal_obj_mem, sizeof(COLOR) * PALETTE_COLORS);
    vid_flip();
  } else
    blindFrames++;

  isReady = false;
}

inline bool sync(u32 local, u32 remote) {
  while (linkSPI->transfer(local) != remote) {
    if (blindFrames >= MAX_BLIND_FRAMES) {
      blindFrames = 0;
      return false;
    }
  }

  return true;
}

inline u32 x() {
  return cursor % RENDER_WIDTH;
}

inline u32 y() {
  return cursor / RENDER_WIDTH;
}
