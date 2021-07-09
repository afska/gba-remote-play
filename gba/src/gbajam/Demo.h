#ifndef DEMO_H
#define DEMO_H

#include <tonc.h>
#include <string>
#include "../Utils.h"
#include "SPIMaster.h"
#include "_state.h"

extern "C" {
#include "gbfs/gbfs.h"
}

namespace Demo {

void send();
void sendMetadata(u32 metadata);
void sendChunk(u32* data, u32* cursor, u32 chunkSize);
u32 read(u32* data, u32* cursor);
void sync(u32 command);
void printOptions();
void printSendOptions();
void printReceiveOptions();
void fail();
void print(std::string text);

static const GBFS_FILE* fs = find_first_gbfs_file(0);
SPIMaster* spiMaster = new SPIMaster();
u32 len, audioLen;
u32* data;
u32* audioData;

CODE_IWRAM void run() {
  u16 keys = 0, previousKeys = 0;
#define IS_PRESSED(KEY) (keys & KEY && !(previousKeys & KEY))

  REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
  tte_init_se(0, BG_CBB(0) | BG_SBB(31), 0xF000, CLR_WHITE, 0, &fwf_default,
              NULL);

  printOptions();

  while (true) {
    keys = pressedKeys();
    softResetIfNeeded(keys);

    if (IS_PRESSED(KEY_START)) {
      // send

      printSendOptions();
      while (true) {
        keys = pressedKeys();
        softResetIfNeeded(keys);

        if (IS_PRESSED(KEY_L)) {
          data = (u32*)gbfs_get_obj(fs, "cam2-video.bin", &len);
          audioData = (u32*)gbfs_get_obj(fs, "cam2-audio.bin", &audioLen);
          send();
        } else if (IS_PRESSED(KEY_R)) {
          data = (u32*)gbfs_get_obj(fs, "cam3-video.bin", &len);
          audioData = (u32*)gbfs_get_obj(fs, "cam3-audio.bin", &audioLen);
          send();
        }

        previousKeys = keys;
        vid_vsync();
      }
    } else if (IS_PRESSED(KEY_SELECT)) {
      // receive

      printReceiveOptions();
      while (true) {
        keys = pressedKeys();
        softResetIfNeeded(keys);

        if (IS_PRESSED(KEY_L)) {
          // none
          state.baseColumn = DRAW_WIDTH / 4;
          state.baseLine = DRAW_HEIGHT / 4;
          state.scaleX = 1;
          state.scaleY = 1;
          state.scanlines = false;
          return;  // (goes back to _main.cpp)
        } else if (IS_PRESSED(KEY_R)) {
          // 2x width
          state.baseColumn = 0;
          state.baseLine = DRAW_HEIGHT / 4;
          state.scaleX = 2;
          state.scaleY = 1;
          state.scanlines = false;
          return;  // (goes back to _main.cpp)
        } else if (IS_PRESSED(KEY_UP)) {
          // scanlines
          state.baseColumn = 0;
          state.baseLine = 0;
          state.scaleX = 2;
          state.scaleY = 2;
          state.scanlines = true;
          return;  // (goes back to _main.cpp)
        }

        previousKeys = keys;
        vid_vsync();
      }
    }

    previousKeys = keys;

    vid_vsync();
  }
}

inline void send() {
  print("Waiting for slave...");

  u32 cursor = 0, audioCursor = 0;
  u32 frame = 0;

  while (true) {
    u16 keys = pressedKeys();
    softResetIfNeeded(keys);

    u32 metadata = read(data, &cursor);
    u32 expectedPackets = (metadata >> PACKS_BIT_OFFSET) & PACKS_BIT_MASK;
    u32 startPixel = metadata & START_BIT_MASK;
    bool hasAudio = (metadata & AUDIO_BIT_MASK) != 0;
    u32 diffStart = (startPixel / 8) / PACKET_SIZE;

    // frame start
    sync(CMD_FRAME_START);

    // send metadata & diffs
    sendMetadata(metadata);
    u32 diffEndPacket = read(data, &cursor);
    spiMaster->transfer(diffEndPacket);
    sendChunk(data, &cursor, diffEndPacket - diffStart);

    if (frame == 0)
      print("Sending...");

    // send audio
    if (hasAudio) {
      sync(CMD_AUDIO);
      sendChunk(audioData, &audioCursor, AUDIO_SIZE_PACKETS);
    }

    // send pixels
    sync(CMD_PIXELS);
    sendChunk(data, &cursor, expectedPackets);

    // frame end
    sync(CMD_FRAME_END);

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
  if ((confirmation = spiMaster->transfer(keys)) != metadata)
    fail();
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

inline void sync(u32 command) {
  u32 local = command + CMD_RPI_OFFSET;
  u32 remote = command + CMD_GBA_OFFSET;
  bool isOnSync = false;

  while (!isOnSync) {
    u32 confirmation = spiMaster->transfer(local);
    isOnSync = confirmation == remote;

    if (confirmation == CMD_RESET)
      fail();
  }
}

inline void printOptions() {
  print(std::string("") +
        "gba-remote-play demo\n\nThis will stream video to the other GBA "
        "at 2Mbps using Link\nCable's Normal Mode.\n\nAs far as I know, it can "
        "only be tested on NO$GBA or \nby using real "
        "hardware.\n\nTo test on hardware:\n\n   >>Use a GBC Link Cable<<"
        "\n\n\n\n\nPress START to send.\n\nPress SELECT to receive.");
}

inline void printSendOptions() {
  print(std::string("") +
        "Nice!\n\nWhat movie do you want?\n\n\n\n\n\n\nL - Caminandes 2: "
        "Gran "
        "Dillama\n\nR - Caminandes 3: Llamigos");
}

inline void printReceiveOptions() {
  print(std::string("") +
        "Perfect!\n\nWhat scale method do you want?\n\n\n\n\n\nL - None "
        "(black borders)\n\nR - 2x width\n\nUP - Scanlines");
}

inline void fail() {
  print(std::string("") +
        "Transfer failed!\n\nIs NO$GBA in Normal Mode?\n\n\n\n\n\n\n\n\n\nIf "
        "you are using hardware:\n\n- Use a GBC Link Cable, not a\n  GBA "
        "cable\n\n- Try again, this works better\n  when using short cables");
  while (true)
    ;
}

inline void print(std::string text) {
  tte_erase_screen();
  tte_write("#{P:0,0}");
  tte_write(text.c_str());
}

}  // namespace Demo

#endif  // DEMO_H
