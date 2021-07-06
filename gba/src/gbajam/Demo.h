#ifndef DEMO_H
#define DEMO_H

#include <tonc.h>
#include <string>
#include "../Utils.h"
#include "SPIMaster.h"

extern "C" {
#include "gbfs/gbfs.h"
}

namespace Demo {

void send();
bool sendMetadata(u32* data, u32* cursor, u32* metadata);
bool sync(u32 command);
void printOptions();
void print(std::string text);

static const GBFS_FILE* fs = find_first_gbfs_file(0);
SPIMaster* spiMaster = new SPIMaster();

CODE_IWRAM void run() {
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
      return;  // (goes back to _main.cpp)
    }

    previousKeys = keys;

    printOptions();
    vid_vsync();
  }
}

inline void send() {
  print("Waiting for remote gba...");

  u32 len;
  u32* data = (u32*)gbfs_get_obj(fs, "record.bin", &len);
  u32 frameStart = 0;

  while (true) {
  reset:
    u32 cursor = frameStart;
    TRY(sync(CMD_FRAME_START))
    u32 metadata;
    TRY(sendMetadata(data, &cursor, &metadata))
  }
}

inline bool sendMetadata(u32* data, u32* cursor, u32* metadata) {
  *metadata = data[*cursor];
  *cursor += PACKET_SIZE;
  u32 keys = spiMaster->transfer(*metadata);
  if (spiMaster->transfer(keys) != *metadata)
    return false;

  print("METADATA OK");
  while (true)
    ;

  return true;
}

inline bool sync(u32 command) {
  u32 local = command + CMD_RPI_OFFSET;
  u32 remote = command + CMD_GBA_OFFSET;
  bool isOnSync = false;

  while (!isOnSync) {
    u32 confirmation = spiMaster->transfer(local);
    isOnSync = confirmation == remote;

    if (confirmation == CMD_RESET)
      return false;
  }

  return true;
}

inline void printOptions() {
  print(std::string("") +
        "gba-remote-play demo\n\nThis will stream video to the other GBA "
        "at 2Mbps using Link\nCable's Normal Mode.\n\nAs far as I know, it can "
        "only be tested on NO$GBA or \nby using real "
        "hardware.\n\n\n\n\n\n\n\n\nPress START to send.\n\nPress SELECT to "
        "receive.");
}

inline void print(std::string text) {
  tte_erase_screen();
  tte_write("#{P:0,0}");
  tte_write(text.c_str());
}

}  // namespace Demo

#endif  // DEMO_H
