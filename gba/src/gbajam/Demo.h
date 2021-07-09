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
void sendMetadata(u32 metadata);
void sendChunk(u32* data, u32* cursor, u32 chunkSize);
u32 read(u32* data, u32* cursor);
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
      // send
      send();
    } else if (IS_PRESSED(KEY_SELECT)) {
      // receive
      return;  // (goes back to _main.cpp)
    }

    previousKeys = keys;

    printOptions();
    vid_vsync();
  }
}

inline void send() {
  print("Waiting for slave...");

  u32 len, audioLen;
  u32* data = (u32*)gbfs_get_obj(fs, "video.bin", &len);
  u32* audioData = (u32*)gbfs_get_obj(fs, "audio.bin", &audioLen);

reset:
  u32 cursor = 0, audioCursor = 0;
  u32 frame = 0;

  while (true) {
    u32 metadata = read(data, &cursor);
    u32 expectedPackets = (metadata >> PACKS_BIT_OFFSET) & PACKS_BIT_MASK;
    u32 startPixel = metadata & START_BIT_MASK;
    bool hasAudio = (metadata & AUDIO_BIT_MASK) != 0;
    u32 diffStart = (startPixel / 8) / PACKET_SIZE;

    // frame start
    if (!sync(CMD_FRAME_START))
      goto reset;

    // send metadata & diffs
    sendMetadata(metadata);
    u32 diffEndPacket = read(data, &cursor);
    sendChunk(data, &cursor, diffEndPacket - diffStart);

    if (frame == 0)
      print("Sending...");

    // send audio
    if (hasAudio) {
      if (!sync(CMD_AUDIO))
        goto reset;
      sendChunk(audioData, &audioCursor, AUDIO_SIZE_PACKETS);
    }

    // send pixels
    if (!sync(CMD_PIXELS))
      goto reset;
    sendChunk(data, &cursor, expectedPackets);

    // frame end
    if (!sync(CMD_FRAME_END))
      goto reset;

    // loop!
    if (cursor * PACKET_SIZE >= len) {
      frame = 0;
      cursor = 0;
      audioCursor = 0;
      audioCursor = 0;
    }

    frame++;
    print("Sending frame #" + std::to_string(frame));
  }
}

inline void sendMetadata(u32 metadata) {
  u32 keys = spiMaster->transfer(metadata);

  u32 confirmation;
  if ((confirmation = spiMaster->transfer(keys)) != metadata) {
    print(std::string("") +
          "Transfer failed!\n\nIs NO$GBA in Normal Mode?\n\n\n\n\n\n\n\n\n\nIf "
          "you are using hardware:\n\n- Use a GB Link Cable, not a\n  GBA "
          "cable\n\n- Try again, use only works\n  reliably with short "
          "cables");
    while (true)
      ;
  }
}

inline void sendChunk(u32* data, u32* cursor, u32 chunkSize) {
  for (u32 i = 0; i < chunkSize; i++)
    spiMaster->transfer(read(data, cursor));
}

inline u32 read(u32* data, u32* cursor) {
  u32 value = data[*cursor];
  (*cursor)++;
  return value;
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
