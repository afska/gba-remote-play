#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include "FrameBuffer.h"
#include "ImageQuantizer.h"
#include "Protocol.h"
#include "SPIMaster.h"
#include "Utils.h"

#define SOURCE_MAX_COLORS 256
#define TARGET_MAX_COLORS 32

class GBARemotePlay {
 public:
  GBARemotePlay() {
    spiMaster = new SPIMaster();
    frameBuffer = new FrameBuffer();
    imageQuantizer = new ImageQuantizer();
  }

  void run() {
    while (true) {
      if (DEBUG) {
        int ret;
        std::cin >> ret;
      }

      // reset flag
      spiMaster->transfer(0x98765432);
      spiMaster->transfer(0x98765432);
      spiMaster->transfer(0x98765432);

      uint8_t* rgbaPixels = frameBuffer->loadFrame();
      auto frame = imageQuantizer->quantize(rgbaPixels, RENDER_WIDTH,
                                            RENDER_HEIGHT, QUANTIZER_SPEED);
      free(frame);
      // TODO: FINISH

      int64_t buffer = -1;
      frameBuffer->forEachPixel([&buffer, this](uint32_t x, uint32_t y,
                                                uint8_t red, uint8_t green,
                                                uint8_t blue) {
        if (x >= RENDER_WIDTH || y >= RENDER_HEIGHT)
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
    }
  }

  ~GBARemotePlay() {
    delete spiMaster;
    delete frameBuffer;
    delete imageQuantizer;
  }

 private:
  SPIMaster* spiMaster;
  FrameBuffer* frameBuffer;
  ImageQuantizer* imageQuantizer;
};

#endif  // GBA_REMOTE_PLAY_H
