#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include "FrameBuffer.h"
#include "ImageQuantizer.h"
#include "Protocol.h"
#include "SPIMaster.h"
#include "Utils.h"

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

      sync(CMD_FRAME_START_RPI, CMD_FRAME_START_GBA);

      uint8_t* rgbaPixels = frameBuffer->loadFrame();
      auto frame = imageQuantizer->quantize(rgbaPixels, RENDER_WIDTH,
                                            RENDER_HEIGHT, QUANTIZER_SPEED);

      if (frame.isValid())
        send(frame);

      frame.clean();
    }
  }

  void send(Frame frame) {
    sync(CMD_PALETTE_START_RPI, CMD_PALETTE_START_GBA);

    for (int i = 0; i < QUANTIZER_COLORS; i += PACKET_SIZE / 2) {
      spiMaster->transfer(frame.raw15bppPalette[i] |
                          frame.raw15bppPalette[i + 1]);
    }

    sync(CMD_PIXELS_START_RPI, CMD_PIXELS_START_GBA);
    for (int i = 0; i < frame.totalPixels; i += PACKET_SIZE) {
      spiMaster->transfer(
          frame.raw8BitPixels[i] | frame.raw8BitPixels[i + 1] << 8 |
          frame.raw8BitPixels[i + 2] << 16 | frame.raw8BitPixels[i + 3] << 24);
    }

    sync(CMD_FRAME_END_RPI, CMD_FRAME_END_GBA);
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

  void sync(uint32_t local, uint32_t remote) {
    while (spiMaster->transfer(local) != remote)
      ;
  }
};

#endif  // GBA_REMOTE_PLAY_H
