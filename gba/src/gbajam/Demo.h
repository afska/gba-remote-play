#ifndef DEMO_H
#define DEMO_H

#include <tonc.h>
#include <string>
#include "../Utils.h"
#include "SPIMaster.h"

extern "C" {
#include "gbfs/gbfs.h"
}

// 16000us/frame and 61,02us per timer tick at TM_FREQ_1024
// in a 20fps video => 50000us per frame => 819ticks per video frame
#define DEMO_SYNC_TIMER 3
#define DEMO_TIMER_TICKS 819
#define DEMO_TIMER_FREQUENCY TM_FREQ_1024
const u16 DEMO_TIMER_IRQ_IDS[] = {IRQ_TIMER0, IRQ_TIMER1, IRQ_TIMER2,
                                  IRQ_TIMER3};

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

bool didTimerCompleted = false;

CODE_IWRAM void ON_TIMER() {
  didTimerCompleted = true;
  REG_TM[DEMO_SYNC_TIMER].cnt = 0;
}

inline void send() {
  print("Waiting for slave...");

  irq_init(NULL);
  irq_add(II_TIMER3, (fnptr)ON_TIMER);

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

    didTimerCompleted = false;
    REG_TM[DEMO_SYNC_TIMER].start = -DEMO_TIMER_TICKS;
    REG_TM[DEMO_SYNC_TIMER].cnt = TM_ENABLE | TM_IRQ | DEMO_TIMER_FREQUENCY;

    // frame start
    if (!sync(CMD_FRAME_START))
      goto reset;

    // send metadata & diffs
    sendMetadata(data, &cursor, metadata);
    sendChunk(data, &cursor, TEMPORAL_DIFF_SIZE / PACKET_SIZE - diffsStart);

    if (frame == 0)
      print("Sending...");

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

    // loop!
    if (cursor * PACKET_SIZE > len)
      cursor = 0;

    frame++;
    // print(std::to_string(frame) + (!didTimerCompleted ? " w" : "") +
    //       (hasAudio ? "a" : ""));

    // if (!didTimerCompleted)
    //   IntrWait(1, DEMO_TIMER_IRQ_IDS[DEMO_SYNC_TIMER]);
  }
}

inline void sendMetadata(u32* data, u32* cursor, u32 metadata) {
  (*cursor)++;
  u32 keys = spiMaster->transfer(metadata);

  u32 confirmation;
  if ((confirmation = spiMaster->transfer(keys)) != metadata) {
    print(std::string("") +
          "Transfer failed!\n\nIs NO$GBA in Normal Mode?\n\n\n\n\n\n\n\n\n\nIf "
          "you are using hardware:\n\n- Use a GB Link Cable, not a\n  GBA "
          "cable\n\n- Try again, this only works\n  reliably with short "
          "cables");
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
