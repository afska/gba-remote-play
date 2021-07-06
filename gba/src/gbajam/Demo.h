#ifndef DEMO_H
#define DEMO_H

#include <tonc.h>
#include <string>
#include "../Utils.h"

extern "C" {
#include "gbfs/gbfs.h"
}

void send();
void printOptions();
void print(std::string text);

static const GBFS_FILE* fs = find_first_gbfs_file(0);

CODE_IWRAM void GbaJamDemo() {
  u16 keys = 0, previousKeys = 0;
#define IS_PRESSED(KEY) (keys & KEY && !(previousKeys & KEY))

  REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
  tte_init_se(0, BG_CBB(0) | BG_SBB(31), 0xF000, CLR_GRAY, 0, &fwf_default,
              NULL);

  while (true) {
    keys = pressedKeys();

    if (IS_PRESSED(KEY_START)) {
      // SEND
      send();
    } else if (IS_PRESSED(KEY_SELECT)) {
      // RECEIVE
      return;  // goes back to _main.cpp
    }

    previousKeys = keys;

    printOptions();
    vid_vsync();
  }
}

void send() {
  // TODO: ASD
}

void printOptions() {
  print(std::string("") +
        "gba-remote-play demo\n\nThis will stream video to the other GBA "
        "at 2Mbps using Link\nCable's Normal Mode.\n\nAs far as I know, it can "
        "only be tested on NO$GBA or \nreal hardware.\n\n\n\n\n\n\n\n\nPress "
        "START to stream.\n\nPress SELECT to receive.");
}

void print(std::string text) {
  tte_erase_screen();
  tte_write("#{P:0,0}");
  tte_write(text.c_str());
}

#endif  // DEMO_H
