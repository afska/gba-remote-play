#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include <stdlib.h>
#include "Config.h"
#include "FrameBuffer.h"
#include "HammingWeight.h"
#include "ImageQuantizer.h"
#include "PNGWriter.h"
#include "Protocol.h"
#include "SPIMaster.h"
#include "TemporalDiffBitArray.h"
#include "VirtualGamepad.h"

class GBARemotePlay {
 public:
  GBARemotePlay() {
    spiMaster = new SPIMaster(SPI_MODE, SPI_FREQUENCY, SPI_DELAY_MICROSECONDS);
    frameBuffer = new FrameBuffer(RENDER_WIDTH, RENDER_HEIGHT);
    imageQuantizer = new ImageQuantizer();
    virtualGamepad = new VirtualGamepad(VIRTUAL_GAMEPAD_NAME);
  }

  void run() {
  reset:
    lastFrame.clean();
    spiMaster->transfer(CMD_RESET);

    while (true) {
#ifdef DEBUG
      LOG("Waiting...");
      int _input;
      std::cin >> _input;
#endif

      uint8_t* rgbaPixels = getRgbaPixels();
      auto frame = imageQuantizer->quantize((uint8_t*)rgbaPixels, RENDER_WIDTH,
                                            RENDER_HEIGHT, QUANTIZER_SPEED);
      free(rgbaPixels);

      if (!send(frame))
        goto reset;

      lastFrame.clean();
      lastFrame = frame;
    }
  }

  ~GBARemotePlay() {
    lastFrame.clean();
    delete spiMaster;
    delete frameBuffer;
    delete imageQuantizer;
    delete virtualGamepad;
  }

 private:
  SPIMaster* spiMaster;
  FrameBuffer* frameBuffer;
  ImageQuantizer* imageQuantizer;
  VirtualGamepad* virtualGamepad;
  Frame lastFrame = Frame{0};

  uint8_t* getRgbaPixels() {
    uint32_t* rgbaPixels = (uint32_t*)malloc(RENDER_WIDTH * RENDER_HEIGHT * 4);

    frameBuffer->forEachPixel(
        [rgbaPixels](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
          rgbaPixels[y * RENDER_WIDTH + x] =
              (0xff << 24) | (b << 16) | (g << 8) | r;
        });

    return (uint8_t*)rgbaPixels;
  }

  bool send(Frame frame) {
    if (!frame.hasData())
      return false;

    DEBULOG("Sending frame start command...");

    if (!sync(CMD_FRAME_START_RPI, CMD_FRAME_START_GBA))
      return false;

    DEBULOG("Calculating diffs...");

    TemporalDiffBitArray diffs;
    diffs.initialize(frame, lastFrame);

    DEBULOG("Exchanging diffs for keys...");

    uint32_t pressedKeys = 0;
    uint32_t pressedKeysCount = 0;
    uint32_t expectedPixels = 0;
    for (int i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++) {
      uint32_t packet = ((uint32_t*)diffs.data)[i];
      uint32_t receivedKeys = spiMaster->transfer(packet);

      expectedPixels += HammingWeight(packet);
      if (pressedKeysCount < PRESSED_KEYS_MIN_VALIDATIONS) {
        if (pressedKeys != receivedKeys) {
          pressedKeys = receivedKeys;
          pressedKeysCount = 0;
        } else
          pressedKeysCount++;
      }
    }
    spiMaster->transfer(expectedPixels);

    if (pressedKeysCount == PRESSED_KEYS_MIN_VALIDATIONS) {
      DEBULOG("Updating pressed keys...");
      virtualGamepad->setKeys(pressedKeys);
    }

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

#ifdef DEBUG
    LOG("Writing debug PNG file...");
    WritePNG("debug.png", frame.raw8BitPixels, frame.raw15bppPalette,
             RENDER_WIDTH, RENDER_HEIGHT);
#endif

    DEBULOG("Frame end!");

    return true;
  }

  bool sync(uint32_t local, uint32_t remote) {
    uint32_t packet = 0;
    while ((packet = spiMaster->transfer(local)) != remote) {
      if (packet == CMD_RESET) {
        LOG("Reset!");
        return false;
      }
    }

    return true;
  }
};

#endif  // GBA_REMOTE_PLAY_H
