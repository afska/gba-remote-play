#ifndef RUNTIME_CONFIG_H
#define RUNTIME_CONFIG_H

#include <tonc.h>
#include "Protocol.h"
#include "Utils.h"
#include "_state.h"

#define CONFIG_ITEMS 7
#define CONFIG_PERCENTAGE_ITEMS 3
#define CONFIG_BOOLEAN_ITEMS 2
#define CONFIG_NUMERIC_ITEMS 9

const char* const CONFIG_PERCENTAGE_OPTIONS[CONFIG_PERCENTAGE_ITEMS] = {
    "<25%>", "<50%>", "<100%>"};
const char* const CONFIG_BOOLEAN_OPTIONS[CONFIG_BOOLEAN_ITEMS] = {"<OFF>",
                                                                  "<ON>"};
const char* const CONFIG_NUMERIC_OPTIONS[CONFIG_NUMERIC_ITEMS] = {
    "<01>", "<02>", "<03>", "<04>", "<05>", "<06>", "<07>", "<08>", "<09>"};

typedef struct Config {
  u32 renderMode;

  u8 frameWidthIndex;
  u8 frameHeightIndex;
  bool scanlines;
  u8 controls;

  void update() {
    renderMode = frameWidthIndex * CONFIG_PERCENTAGE_ITEMS + frameHeightIndex;
  }
} Config;

extern Config config;

namespace RuntimeConfig {

enum Option {
  FRAME_WIDTH,
  FRAME_HEIGHT,
  SCANLINES,
  CONTROLS,
  BENCHMARK,
  DEFAULTS,
  START
};

inline void drawMenu(u32 option) {
#define SELECTION(ID) (option == ID ? ">" : " ")

  tte_erase_screen();
  tte_write("#{P:0,0}");
  tte_write(" gba-remote-play        (^_^)\n\n");
  tte_write(SELECTION(Option::FRAME_WIDTH));
  tte_write("Frame width            ");
  tte_write(CONFIG_PERCENTAGE_OPTIONS[config.frameWidthIndex]);
  tte_write("\n");
  tte_write(SELECTION(Option::FRAME_HEIGHT));
  tte_write("Frame height           ");
  tte_write(CONFIG_PERCENTAGE_OPTIONS[config.frameHeightIndex]);
  tte_write("\n");
  tte_write(SELECTION(Option::SCANLINES));
  tte_write("Scanlines              ");
  tte_write(CONFIG_BOOLEAN_OPTIONS[config.scanlines]);
  tte_write("\n");
  tte_write(SELECTION(Option::CONTROLS));
  tte_write("Controls               ");
  tte_write(CONFIG_NUMERIC_OPTIONS[config.controls]);
  tte_write("\n\n");
  tte_write(SELECTION(Option::BENCHMARK));
  tte_write("[BENCHMARK]\n");
  tte_write(SELECTION(Option::DEFAULTS));
  tte_write("[RESET OPTIONS]\n");
  tte_write(SELECTION(Option::START));
  tte_write("[START STREAMING]");
}

inline void initialize() {
  config.frameWidthIndex = 1;
  config.frameHeightIndex = 1;
  config.scanlines = true;
  config.controls = 0;
  config.update();
}

inline void show() {
  u32 option = 0;
  u32 keys = 0, previousKeys = pressedKeys();

#define IS_PRESSED(KEY) (keys & KEY && !(previousKeys & KEY))
#define CYCLE_OPTIONS(N, LIMIT) ((N) < 0 ? ((LIMIT)-1) : ((N) % LIMIT))

  REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
  tte_init_se(0, BG_CBB(0) | BG_SBB(31), 0xF000, CLR_WHITE, 0, &fwf_default,
              NULL);

  drawMenu(option);

  while (true) {
    keys = pressedKeys();

    if (IS_PRESSED(KEY_DOWN)) {
      option = (option + 1) % CONFIG_ITEMS;
      drawMenu(option);
    } else if (IS_PRESSED(KEY_UP)) {
      option = option == 0 ? CONFIG_ITEMS - 1 : option - 1;
      drawMenu(option);
    } else if (IS_PRESSED(KEY_A) || IS_PRESSED(KEY_RIGHT) ||
               IS_PRESSED(KEY_LEFT)) {
      s8 direction = (IS_PRESSED(KEY_LEFT) ? -1 : 1);

      switch (option) {
        case Option::FRAME_WIDTH: {
          config.frameWidthIndex = CYCLE_OPTIONS(
              config.frameWidthIndex + direction, CONFIG_PERCENTAGE_ITEMS);
          config.update();
          break;
        }
        case Option::FRAME_HEIGHT: {
          config.frameHeightIndex = CYCLE_OPTIONS(
              config.frameHeightIndex + direction, CONFIG_PERCENTAGE_ITEMS);
          config.update();
          break;
        }
        case Option::SCANLINES: {
          config.scanlines = !config.scanlines;
          break;
        }
        case Option::CONTROLS: {
          config.controls =
              CYCLE_OPTIONS(config.controls + direction, CONFIG_NUMERIC_ITEMS);
          break;
        }
        case Option::DEFAULTS: {
          if (IS_PRESSED(KEY_A))
            initialize();
          break;
        }
        case Option::START: {
          if (IS_PRESSED(KEY_A))
            return;
          break;
        }
      }
      drawMenu(option);
    } else if (IS_PRESSED(KEY_START)) {
      return;
    }

    previousKeys = keys;
    vid_vsync();
  }
}

}  // namespace RuntimeConfig

#endif  // RUNTIME_CONFIG_H
