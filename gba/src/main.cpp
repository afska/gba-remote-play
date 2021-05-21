#include <tonc.h>

#include <string>

void init();
void log(std::string text);

#define IS_MASTER true
#define BIT_CLOCK 0
#define BIT_SI 2
#define BIT_SO 3
#define BIT_START 7
#define BIT_LENGTH 12

int main() {
  init();

  log("Hello world");

  u32 step = 0;

  while (true) {
  next:
    if (step == 0) {
      REG_RCNT = 0;
      log("Set RCNT=0 (NORMAL)");
      step++;
      goto next;
    } else if (step == 1) {
      REG_SIOCNT = 0;
      log("Set SIOCNT=0 (256KHZ)");
      step++;
      goto next;
    } else if (step == 2) {
      REG_SIOCNT |= 1 << BIT_LENGTH;
      log("Set SIOCNT<12>=1 (32bit)");
      step++;
      goto next;
    } else if (step == 3) {
      REG_SIODATA32 = 0x12345678;
      log("Set SIODATA32=...");
      step++;
      goto next;
    } else if (step == 4) {
      REG_SIOCNT |= 1 << BIT_SO;
      log("Set SIOCNT<3>=1 (SO=HI)");
      step++;
      goto next;
    } else if (step == 5) {
      REG_SIOCNT &= ~(1 << BIT_CLOCK);
      log("Set SIOCNT<0>=0 (EXT-slve)");
      step++;
      goto next;
    } else if (step == 6) {
      REG_SIOCNT &= ~(1 << BIT_SO);
      log("Set SIOCNT<3>=0 (SO=LOW");
      step++;
      goto next;
    } else if (step == 7) {
      REG_SIOCNT |= 1 << BIT_START;
      log("Set SIOCNT<7>=1 (START=HI)");
      log("Waiting...");
      step++;
      goto next;
    } else if (step == 8) {
      if ((REG_SIOCNT & (1 << BIT_START)) == 0) {
        log("(!) START low");
        log("Data: " + std::to_string(REG_SIODATA32));
        step++;
      }
    }

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
