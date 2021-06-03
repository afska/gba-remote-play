#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include "Config.h"
#include "FrameBuffer.h"
#include "ImageQuantizer.h"
#include "PNGWriter.h"
#include "Protocol.h"
#include "SPIMaster.h"
#include "TemporalDiffBitArray.h"

class GBARemotePlay {
 public:
  GBARemotePlay() {
    spiMaster = new SPIMaster(SPI_MODE, SPI_FREQUENCY, SPI_DELAY_MICROSECONDS);
    frameBuffer = new FrameBuffer(RENDER_WIDTH, RENDER_HEIGHT);
    imageQuantizer = new ImageQuantizer();
  }

  void run() {
  reset:
    spiMaster->transfer(CMD_RESET);

    while (true) {
      if (DEBUG) {
        int ret;
        DEBULOG("Waiting...");
        std::cin >> ret;
        DEBULOG("Sending start command...");
      }

      if (!sync(CMD_FRAME_START_RPI, CMD_FRAME_START_GBA))
        goto reset;

      DEBULOG("Loading frame...");
      uint8_t* rgbaPixels = frameBuffer->loadFrame();
      auto frame = imageQuantizer->quantize(rgbaPixels, RENDER_WIDTH,
                                            RENDER_HEIGHT, QUANTIZER_SPEED);

      if (!send(frame))
        goto reset;

      lastFrame.clean();
      lastFrame = frame;
    }
  }

  bool send(Frame frame) {
    if (!frame.hasData())
      return false;

    DEBULOG("Calculating diffs...");
    TemporalDiffBitArray diffs;
    diffs.initialize(frame, lastFrame);

    DEBULOG("Sending diffs...");
    for (int i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++)
      spiMaster->transfer(((uint32_t*)diffs.data)[i]);

    DEBULOG("Sending palette command...");
    if (!sync(CMD_PALETTE_START_RPI, CMD_PALETTE_START_GBA))
      return false;

    DEBULOG("Sending palette...");
    for (int i = 0; i < PALETTE_COLORS / COLORS_PER_PACKET; i++)
      spiMaster->transfer(((uint32_t*)frame.raw15bppPalette)[i]);

    DEBULOG("Sending pixels command...");
    if (!sync(CMD_PIXELS_START_RPI, CMD_PIXELS_START_GBA))
      return false;

    DEBULOG("Sending pixels...");
    uint32_t outgoingPacket = 0;
    uint8_t byte = 0;
    for (int i = 0; i < frame.totalPixels; i++) {
      if (frame.hasPixelChanged(i, lastFrame)) {
        outgoingPacket |= frame.raw8BitPixels[i] << (byte * 8);

        byte++;
        if (byte == PACKET_SIZE || i == frame.totalPixels - 1) {
          spiMaster->transfer(outgoingPacket);
          outgoingPacket = 0;
          byte = 0;
        }
      }
    }

    DEBULOG("Sending end command...");
    if (!sync(CMD_FRAME_END_RPI, CMD_FRAME_END_GBA))
      return false;

    if (DEBUG) {
      DEBULOG("Writing debug PNG file...");
      WritePNG("debug.png", frame.raw8BitPixels, frame.raw15bppPalette,
               RENDER_WIDTH, RENDER_HEIGHT);
    }

    DEBULOG("Frame end!");
    return true;
  }

  ~GBARemotePlay() {
    lastFrame.clean();
    delete spiMaster;
    delete frameBuffer;
    delete imageQuantizer;
  }

 private:
  SPIMaster* spiMaster;
  FrameBuffer* frameBuffer;
  ImageQuantizer* imageQuantizer;
  Frame lastFrame = Frame{0};

  bool sync(uint32_t local, uint32_t remote) {
    uint32_t packet = 0;
    while ((packet = spiMaster->transfer(local)) != remote) {
      if (packet == CMD_RESET) {
        DEBULOG("Reset!");
        return false;
      }
    }

    return true;
  }
};

#endif  // GBA_REMOTE_PLAY_H
