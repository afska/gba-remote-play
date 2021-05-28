#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include "LinuxFrameBuffer.h"
#include "SPIMaster.h"
#include "Utils.h"

#define SOURCE_MAX_COLORS 256
#define TARGET_MAX_COLORS 32
#define GBA_WIDTH 120
#define GBA_HEIGHT 80

class GBARemotePlay {
 public:
  GBARemotePlay() {
    spiMaster = new SPIMaster();
    frameBuffer = new LinuxFrameBuffer();
  }

  void run() {
    while (true) {
      if (DEBUG) {
        int ret;
        std::cin >> ret;
      }

      // reset flag
      spiMaster->transfer(even ? 0x98765432 : 0x98765431);
      spiMaster->transfer(even ? 0x98765432 : 0x98765431);
      spiMaster->transfer(even ? 0x98765432 : 0x98765431);

      int64_t buffer = -1;
      frameBuffer->forEachPixel([&buffer, this](uint32_t x, uint32_t y,
                                                uint8_t red, uint8_t green,
                                                uint8_t blue) {
        if (x >= GBA_WIDTH || y >= GBA_HEIGHT)
          return;

        bool isEven = y % 2 == 0;
        if (even != isEven)
          return;

        uint8_t targetRed = red * TARGET_MAX_COLORS / SOURCE_MAX_COLORS;
        uint8_t targetGreen = green * TARGET_MAX_COLORS / SOURCE_MAX_COLORS;
        uint8_t targetBlue = blue * TARGET_MAX_COLORS / SOURCE_MAX_COLORS;
        uint16_t target = targetRed | (targetGreen << 5) | (targetBlue << 10);

        if (buffer == -1)
          buffer = target;
        else {
          spiMaster->transfer(buffer | (target << 16));
          buffer = -1;
        }
      });

      even = !even;
    }
  }

  ~GBARemotePlay() {
    delete spiMaster;
    delete frameBuffer;
  }

 private:
  SPIMaster* spiMaster;
  LinuxFrameBuffer* frameBuffer;
  bool even = true;
};

#endif  // GBA_REMOTE_PLAY_H
