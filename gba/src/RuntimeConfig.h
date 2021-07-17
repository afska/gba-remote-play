#ifndef RUNTIME_CONFIG_H
#define RUNTIME_CONFIG_H

#include <tonc.h>
#include <string>
#include "Utils.h"
#include "_state.h"

#define CONFIG_ITEMS 7
#define CONFIG_PERCENTAGE_ITEMS 3
#define CONFIG_BOOLEAN_ITEMS 2
#define CONFIG_NUMERIC_ITEMS 9
#define CONFIG_RENDER_MODES 9

const u32 CONFIG_RENDER_MODE_WIDTH[CONFIG_RENDER_MODES] = {
    60, 60, 60, 120, 120, 120, 240, 240, 240};
const u32 CONFIG_RENDER_MODE_HEIGHT[CONFIG_RENDER_MODES] = {
    40, 80, 160, 40, 80, 160, 40, 80, 160};
const u32 CONFIG_RENDER_MODE_SCALEX[CONFIG_RENDER_MODES] = {4, 4, 4, 2, 2,
                                                            2, 1, 1, 1};
const u32 CONFIG_RENDER_MODE_SCALEY[CONFIG_RENDER_MODES] = {4, 2, 1, 4, 2,
                                                            1, 4, 2, 1};
const u32 CONFIG_RENDER_MODE_PIXELS[CONFIG_RENDER_MODES] = {
    2400, 4800, 9600, 4800, 9600, 19200, 9600, 19200, 38400};

const std::string CONFIG_PERCENTAGE_OPTIONS[CONFIG_PERCENTAGE_ITEMS] = {
    "<25%>", "<50%>", "<100%>"};
const std::string CONFIG_BOOLEAN_OPTIONS[CONFIG_BOOLEAN_ITEMS] = {"<OFF>",
                                                                  "<ON>"};
const std::string CONFIG_NUMERIC_OPTIONS[CONFIG_NUMERIC_ITEMS] = {
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

inline void print(std::string text) {
  tte_erase_screen();
  tte_write("#{P:0,0}");
  tte_write(text.c_str());
}

inline void drawMenu(u32 option) {
#define SELECTION(ID) (option == ID ? ">" : " ")

  print(std::string(" gba-remote-play        (^_^)\n\n") +
        SELECTION(Option::FRAME_WIDTH) + "Frame width            " +
        CONFIG_PERCENTAGE_OPTIONS[config.frameWidthIndex] + "\n" +
        SELECTION(Option::FRAME_HEIGHT) + "Frame height           " +
        CONFIG_PERCENTAGE_OPTIONS[config.frameHeightIndex] + "\n" +
        SELECTION(Option::SCANLINES) + "Scanlines              " +
        CONFIG_BOOLEAN_OPTIONS[config.scanlines] + "\n" +
        SELECTION(Option::CONTROLS) + "Controls               " +
        CONFIG_NUMERIC_OPTIONS[config.controls] + "\n" + "\n" +
        SELECTION(Option::BENCHMARK) + "[BENCHMARK]" + "\n" +
        SELECTION(Option::DEFAULTS) + "[RESET OPTIONS]" + "\n" +
        SELECTION(Option::START) + "[START STREAMING]");
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
