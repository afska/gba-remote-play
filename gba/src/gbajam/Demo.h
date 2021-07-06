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
void sendMetadata(u32* data, u32* cursor, u32 metadata);
void sendChunk(u32* data, u32* cursor, u32 chunkSize);
bool sync(u32 command);
void printOptions();
void print(std::string text);

static const GBFS_FILE* fs = find_first_gbfs_file(0);
SPIMaster* spiMaster = new SPIMaster();

CODE_IWRAM void run() {
  u16 keys = 0, previousKeys = 0;
#define IS_PRESSED(KEY) (keys & KEY && !(previousKeys & KEY))

  REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
  tte_init_se(0, BG_CBB(0) | BG_SBB(31), 0xF000, CLR_WHITE, 0, &fwf_default,
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
  print("Waiting for slave...");

  u32 len;
  u32* data = (u32*)gbfs_get_obj(fs, "record.bin", &len);

reset:
  u32 cursor = 0;
  u32 frame = 0;

  while (true) {
    u32 metadata = data[cursor];
    u32 expectedPackets = (metadata >> PACKS_BIT_OFFSET) & PACKS_BIT_MASK;
    u32 startPixel = metadata & START_BIT_MASK;
    bool hasAudio = (metadata & AUDIO_BIT_MASK) != 0;
    u32 diffsStart = (startPixel / 8) / PACKET_SIZE;

    // frame start
    if (!sync(CMD_FRAME_START))
      goto reset;

    // send metadata & diffs
    sendMetadata(data, &cursor, metadata);
    sendChunk(data, &cursor, TEMPORAL_DIFF_SIZE / PACKET_SIZE - diffsStart);

    // send audio
    if (hasAudio) {
      if (!sync(CMD_AUDIO))
        goto reset;
      sendChunk(data, &cursor, AUDIO_SIZE_PACKETS);
    }

    // send pixels
    if (!sync(CMD_PIXELS))
      goto reset;
    sendChunk(data, &cursor, expectedPackets);

    // frame end
    if (!sync(CMD_FRAME_END))
      goto reset;

    frame++;
    if (frame % 60 == 0)
      print(std::to_string(frame));

    // loop!
    if (cursor * PACKET_SIZE > len)
      cursor = 0;
  }
}

inline void sendMetadata(u32* data, u32* cursor, u32 metadata) {
  (*cursor)++;
  u32 keys = spiMaster->transfer(metadata);

  u32 confirmation;
  if ((confirmation = spiMaster->transfer(keys)) != metadata) {
    print(std::string("") + "Error!\nAre you using Normal Mode?\n\n" +
          "Received: " + std::to_string(confirmation));
    while (true)
      ;
  }
}

inline void sendChunk(u32* data, u32* cursor, u32 chunkSize) {
  for (u32 i = 0; i < chunkSize; i++) {
    spiMaster->transfer(data[*cursor]);
    (*cursor)++;
  }
}

inline bool sync(u32 command) {
  u32 local = command + CMD_RPI_OFFSET;
  u32 remote = command + CMD_GBA_OFFSET;
  bool isOnSync = false;

  while (!isOnSync) {
    u32 confirmation = spiMaster->transfer(local);
    isOnSync = confirmation == remote;

    if (confirmation == CMD_RESET) {
      print("RESET");
      while (true)
        ;
      return false;
    }
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
