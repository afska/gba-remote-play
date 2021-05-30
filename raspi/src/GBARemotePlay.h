#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include "Config.h"
#include "FrameBuffer.h"
#include "ImageQuantizer.h"
#include "Protocol.h"
#include "SPIMaster.h"

class GBARemotePlay {
 public:
  GBARemotePlay() {
    spiMaster = new SPIMaster();
    frameBuffer = new FrameBuffer();
    imageQuantizer = new ImageQuantizer();
  }

  void run() {
    while (true) {
    reset:
      if (DEBUG) {
        int ret;
        std::cout << "Waiting...\n";
        std::cin >> ret;
        std::cout << "Sending start command...\n";
      }

      spiMaster->transfer(CMD_RESET);

      if (!sync(CMD_FRAME_START_RPI, CMD_FRAME_START_GBA))
        goto reset;

      if (DEBUG)
        std::cout << "Loading frame...\n";

      uint8_t* rgbaPixels = frameBuffer->loadFrame();
      auto frame = imageQuantizer->quantize(rgbaPixels, RENDER_WIDTH,
                                            RENDER_HEIGHT, QUANTIZER_SPEED);

      if (frame.isValid())
        send(frame);

      frame.clean();
    }
  }

  void send(Frame frame) {
    if (DEBUG)
      std::cout << "Sending palette...\n";

    for (int i = 0; i < PALETTE_COLORS; i += COLORS_PER_PACKET)
      spiMaster->transfer(frame.raw15bppPalette[i] |
                          (frame.raw15bppPalette[i + 1] << 16));

    if (DEBUG)
      std::cout << "Sending pixels command...\n";

    if (!sync(CMD_PIXELS_START_RPI, CMD_PIXELS_START_GBA))
      return;

    if (DEBUG)
      std::cout << "Sending pixels...\n";

    for (int i = 0; i < frame.totalPixels; i += PIXELS_PER_PACKET)
      spiMaster->transfer(frame.raw8BitPixels[i] |
                          (frame.raw8BitPixels[i + 1] << 8) |
                          (frame.raw8BitPixels[i + 2] << 16) |
                          (frame.raw8BitPixels[i + 3] << 24));

    if (DEBUG)
      std::cout << "Sending end command...\n";

    if (!sync(CMD_FRAME_END_RPI, CMD_FRAME_END_GBA))
      return;

    if (DEBUG)
      std::cout << "Frame end!\n";
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

  bool sync(uint32_t local, uint32_t remote) {
    uint32_t packet = 0;
    while ((packet = spiMaster->transfer(local)) != remote) {
      if (packet == CMD_RESET) {
        std::cout << "Reset!\n";
        return false;
      }
    }

    return true;
  }
};

#endif  // GBA_REMOTE_PLAY_H
