#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include "LinuxFrameBuffer.h"
#include "SPIMaster.h"

#define CURRENT_MAX_COLORS 256
#define TARGET_MAX_COLORS 32

class GBARemotePlay {
 public:
  GBARemotePlay() {
    spiMaster = new SPIMaster();
    frameBuffer = new LinuxFrameBuffer();
  }

  void run() {
    while (true) {
      // reset flag
      spiMaster->transfer(0x98765432);
      spiMaster->transfer(0x98765432);
      spiMaster->transfer(0x98765432);

      int64_t buffer = -1;
      frameBuffer->forEachPixel([&buffer, this](uint32_t red, uint32_t green,
                                                uint32_t blue) {
        uint8_t targetRed = red * TARGET_MAX_COLORS / CURRENT_MAX_COLORS;
        uint8_t targetGreen = green * TARGET_MAX_COLORS / CURRENT_MAX_COLORS;
        uint8_t targetBlue = blue * TARGET_MAX_COLORS / CURRENT_MAX_COLORS;
        uint16_t target = targetRed | (targetGreen << 5) | (targetBlue << 10);

        if (buffer == -1)
          buffer = target;
        else {
          spiMaster->transfer(buffer | (target << 16));
          buffer = -1;
        }
      });
    }
  }

  ~GBARemotePlay() {
    delete spiMaster;
    delete frameBuffer;
  }

 private:
  SPIMaster* spiMaster;
  LinuxFrameBuffer* frameBuffer;
};

#endif  // GBA_REMOTE_PLAY_H
