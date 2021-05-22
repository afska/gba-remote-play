#include <tonc.h>

#include <string>
#include "LinkSPI.h"

void init();
void log(std::string text);

LinkSPI* linkSPI = new LinkSPI();

int main() {
  init();

  log("Waiting...");
  log(std::to_string(linkSPI->transfer(0x12345678)));

  while (true) {
    VBlankIntrWait();
  }

  return 0;
}

void init() {
  REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
  tte_init_se_default(0, BG_CBB(0) | BG_SBB(31));

  irq_init(NULL);
  irq_add(II_VBLANK, NULL);
}

void log(std::string text) {
  tte_erase_screen();
  tte_write("#{P:0,0}");
  tte_write(text.c_str());
}
