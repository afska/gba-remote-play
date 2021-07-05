#ifndef CONFIG_H
#define CONFIG_H

#include <tonc.h>
#include <string>
#include "Utils.h"
#include "_state.h"

#define CONFIG_ITEMS 6
#define CONFIG_PERCENTAGE_ITEMS 3
#define CONFIG_BOOLEAN_ITEMS 2
#define CONFIG_NUMERIC_ITEMS 9

typedef struct Config {
  u32 frameWidth;
  u32 frameHeight;
  bool scanlines;
  bool audio;
  u8 controls;
} Config;

const u32 CONFIG_FRAME_WIDTH_OPTIONS[CONFIG_PERCENTAGE_ITEMS] = {60, 120, 240};
const u32 CONFIG_FRAME_HEIGHT_OPTIONS[CONFIG_PERCENTAGE_ITEMS] = {40, 80, 160};

const std::string CONFIG_PERCENTAGE_OPTIONS[CONFIG_PERCENTAGE_ITEMS] = {
    "<25%>", "<50%>", "<100%>"};
const std::string CONFIG_BOOLEAN_OPTIONS[CONFIG_BOOLEAN_ITEMS] = {"<OFF>",
                                                                  "<ON>"};
const std::string CONFIG_NUMERIC_OPTIONS[CONFIG_NUMERIC_ITEMS] = {
    "<01>", "<02>", "<03>", "<04>", "<05>", "<06>", "<07>", "<08>", "<09>"};

void print(std::string text);

inline void ConfigMenu() {
  u32 option = 0;
  u32 keys = 0, previousKeys = 0;

#define IS_PRESSED(KEY) (keys & KEY && !(previousKeys & KEY))
#define SELECTION(ID) (option == ID ? ">" : " ")
#define DRAW_MENU()                                                        \
  print(std::string("gba-remote-play          (^_^)\n\n") + SELECTION(0) + \
        "Frame width             <50%>\n" + SELECTION(1) +                 \
        "Frame height            <50%>\n" + SELECTION(2) +                 \
        "Scanlines               <ON>\n" + SELECTION(3) +                  \
        "Audio                   <ON>\n" + SELECTION(4) +                  \
        "Controls                <01>\n" + "\n" + SELECTION(5) + "[RESTART]");

  REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
  tte_init_se(0, BG_CBB(0) | BG_SBB(31), 0xF000, CLR_WHITE, 0, &fwf_default,
              NULL);

  while (true) {
    keys = pressedKeys();

    if (IS_PRESSED(KEY_DOWN)) {
      option = (option + 1) % CONFIG_ITEMS;
    } else if (IS_PRESSED(KEY_UP)) {
      option = option == 0 ? CONFIG_ITEMS - 1 : option - 1;
    } else if (IS_PRESSED(KEY_LEFT) || IS_PRESSED(KEY_RIGHT)) {
      // if ()
    }

    previousKeys = keys;

    DRAW_MENU()

    vid_vsync();
  }
}

void print(std::string text) {
  tte_erase_screen();
  tte_write("#{P:0,0}");
  tte_write(text.c_str());
}

#endif  // CONFIG_H
