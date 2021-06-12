#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include "Config.h"
#include "Frame.h"
#include "FrameBuffer.h"
#include "ImageDiffBitArray.h"
#include "PNGWriter.h"
#include "Palette.h"
#include "Protocol.h"
#include "SPIMaster.h"
#include "VirtualGamepad.h"

uint8_t LUT_24BPP_TO_8BIT_PALETTE[PALETTE_24BIT_MAX_COLORS];

class GBARemotePlay {
 public:
  GBARemotePlay() {
    spiMaster = new SPIMaster(SPI_MODE, SPI_SLOW_FREQUENCY, SPI_FAST_FREQUENCY,
                              SPI_DELAY_MICROSECONDS);
    frameBuffer = new FrameBuffer(RENDER_WIDTH, RENDER_HEIGHT);
    virtualGamepad = new VirtualGamepad(VIRTUAL_GAMEPAD_NAME);
    lastFrame = Frame{0};
    resetKeys();

    PALETTE_initializeCache(PALETTE_CACHE_FILENAME);
  }

  void run() {
  reset:
    lastFrame.clean();
    resetKeys();
    spiMaster->send(CMD_RESET);

#ifdef PROFILE
    auto startTime = PROFILE_START();
    uint32_t frames = 0;
#endif

    while (true) {
#ifdef DEBUG
      LOG("Waiting...");
      int _input;
      std::cin >> _input;
#endif

#ifdef PROFILE_VERBOSE
      auto frameGenerationStartTime = PROFILE_START();
#endif

      auto frame = loadFrame();

#ifdef PROFILE_VERBOSE
      auto frameGenerationElapsedTime = PROFILE_END(frameGenerationStartTime);
      auto frameDiffsStartTime = PROFILE_START();
#endif

      ImageDiffBitArray diffs;
      diffs.initialize(frame, lastFrame);

#ifdef PROFILE_VERBOSE
      auto frameDiffsElapsedTime = PROFILE_END(frameDiffsStartTime);
      auto frameTransferStartTime = PROFILE_START();
#endif

      if (!send(frame, diffs))
        goto reset;

      lastFrame.clean();
      lastFrame = frame;

#ifdef PROFILE_VERBOSE
      auto frameTransferElapsedTime = PROFILE_END(frameTransferStartTime);
      std::cout << "(build: " + std::to_string(frameGenerationElapsedTime) +
                       "ms, diffs: " + std::to_string(frameDiffsElapsedTime) +
                       "ms, transfer: " +
                       std::to_string(frameTransferElapsedTime) + "ms)\n";
#endif

#ifdef PROFILE
      frames++;
      uint32_t elapsedTime = PROFILE_END(startTime);
      if (elapsedTime >= 1000) {
        std::cout << "--- " + std::to_string(frames) + " frames ---\n";
        startTime = PROFILE_START();
        frames = 0;
      }
#endif
    }
  }

  ~GBARemotePlay() {
    lastFrame.clean();
    delete spiMaster;
    delete frameBuffer;
    delete virtualGamepad;
  }

 private:
  SPIMaster* spiMaster;
  FrameBuffer* frameBuffer;
  VirtualGamepad* virtualGamepad;
  Frame lastFrame;
  uint32_t input;
  uint32_t inputValidations;

  bool send(Frame& frame, ImageDiffBitArray& diffs) {
    if (!frame.hasData())
      return false;

#ifdef PROFILE_VERBOSE
    auto idleStartTime = PROFILE_START();
#endif

    DEBULOG("Sending frame start command...");
    if (!sync(CMD_FRAME_START_RPI, CMD_FRAME_START_GBA))
      return false;

#ifdef PROFILE_VERBOSE
    auto idleElapsedTime = PROFILE_END(idleStartTime);
    std::cout << "  <" + std::to_string(idleElapsedTime) + "ms idle>\n";
#endif

    DEBULOG("Sending diffs...");
    sendDiffs(diffs);

    DEBULOG("Sending pixels command...");
    if (!sync(CMD_PIXELS_START_RPI, CMD_PIXELS_START_GBA))
      return false;

    DEBULOG("Sending pixels...");
    sendPixels(frame, diffs);

    DEBULOG("Sending end command...");
    if (!sync(CMD_FRAME_END_RPI, CMD_FRAME_END_GBA))
      return false;

#ifdef DEBUG
    LOG("Writing debug PNG file...");
    WritePNG("debug.png", frame.raw8BitPixels, MAIN_PALETTE_24BPP, RENDER_WIDTH,
             RENDER_HEIGHT);
    LOG("Frame end!");
#endif

    return true;
  }

  void sendDiffs(ImageDiffBitArray& diffs) {
    for (int i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++) {
      uint32_t packet = ((uint32_t*)diffs.temporal)[i];

      if (i < PRESSED_KEYS_REPETITIONS) {
        uint32_t receivedKeys = spiMaster->exchange(packet);
        processKeys(receivedKeys);
      } else
        spiMaster->send(packet);
    }

    spiMaster->send(diffs.compressedPixels / PIXELS_PER_PACKET +
                    diffs.compressedPixels % PIXELS_PER_PACKET);

    for (int i = 0; i < SPATIAL_DIFF_SIZE / PACKET_SIZE; i++)
      spiMaster->send(((uint32_t*)diffs.spatial)[i]);
  }

  void sendPixels(Frame& frame, ImageDiffBitArray& diffs) {
    uint32_t compressedPixelId = 0;
    uint32_t outgoingPacket = 0;
    uint8_t byte = 0;

    for (int i = 0; i < frame.totalPixels; i++) {
      if (diffs.hasPixelChanged(i)) {
        uint32_t block = compressedPixelId / SPATIAL_DIFF_BLOCK_SIZE;
        uint32_t blockPart = compressedPixelId % SPATIAL_DIFF_BLOCK_SIZE;
        compressedPixelId++;
        if (blockPart > 1 && diffs.isRepeatedBlock(block))
          continue;

        outgoingPacket |= frame.raw8BitPixels[i] << (byte * 8);
        byte++;
        if (byte == PACKET_SIZE) {
          spiMaster->send(outgoingPacket);
          outgoingPacket = 0;
          byte = 0;
        }
      }
    }

    if (byte > 0)
      spiMaster->send(outgoingPacket);
  }

  void processKeys(uint16_t receivedKeys) {
    if (input != receivedKeys) {
      input = receivedKeys;
      inputValidations = 0;
    } else
      inputValidations++;

    if (inputValidations == PRESSED_KEYS_MIN_VALIDATIONS) {
      virtualGamepad->setKeys(input);
      resetKeys();
    }
  }

  void resetKeys() {
    input = 0xffffffff;
    inputValidations = 0;
  }

  bool sync(uint32_t local, uint32_t remote) {
    uint32_t packet = 0;
    uint32_t lastPacket = 0;
    while ((packet = spiMaster->exchange(local)) != remote) {
      if (packet == CMD_RESET) {
        LOG("Reset!");
        std::cout << std::hex << local << "\n";
        std::cout << std::hex << lastPacket << "\n\n";
        return false;
      }
      lastPacket = packet;
    }

    return true;
  }

  Frame loadFrame() {
    Frame frame;
    frame.totalPixels = TOTAL_PIXELS;
    frame.raw8BitPixels = (uint8_t*)malloc(TOTAL_PIXELS);
    frame.palette = MAIN_PALETTE_24BPP;

    frameBuffer->forEachPixel(
        [&frame](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
          frame.raw8BitPixels[y * RENDER_WIDTH + x] =
              LUT_24BPP_TO_8BIT_PALETTE[(r << 0) | (g << 8) | (b << 16)];
        });

    return frame;
  }
};

#endif  // GBA_REMOTE_PLAY_H
