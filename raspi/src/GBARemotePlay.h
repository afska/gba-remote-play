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

      if (!send(frame, diffs)) {
        frame.clean();
        goto reset;
      }

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
      if (elapsedTime >= ONE_SECOND) {
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
    if (!sync(CMD_FRAME_START))
      return false;

#ifdef PROFILE_VERBOSE
    auto idleElapsedTime = PROFILE_END(idleStartTime);
    std::cout << "  <" + std::to_string(idleElapsedTime) + "ms idle>\n";
#endif

    DEBULOG("Receiving keys and sending temporal diffs...");
    if (!receiveKeysAndSendTemporalDiffs(diffs))
      return false;

    DEBULOG("Sending spatial diffs start command...");
    if (!sync(CMD_SPATIAL_DIFFS_START))
      return false;

    DEBULOG("Sending spatial diffs...");
    if (!sendSpatialDiffs(diffs))
      return false;

    DEBULOG("Sending pixels command...");
    if (!sync(CMD_PIXELS_START))
      return false;

    DEBULOG("Sending pixels...");
    if (!sendPixels(frame, diffs))
      return false;

    DEBULOG("Sending end command...");
    if (!sync(CMD_FRAME_END))
      return false;

#ifdef DEBUG
    LOG("Writing debug PNG file...");
    WritePNG("debug.png", frame.raw8BitPixels, MAIN_PALETTE_24BPP, RENDER_WIDTH,
             RENDER_HEIGHT);
    LOG("Frame end!");
#endif

    return true;
  }

  bool receiveKeysAndSendTemporalDiffs(ImageDiffBitArray& diffs) {
    spiMaster->send(diffs.compressedPixels / PIXELS_PER_PACKET +
                    diffs.compressedPixels % PIXELS_PER_PACKET);

    for (int i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++) {
      uint32_t packet = ((uint32_t*)diffs.temporal)[i];

      if (i < PRESSED_KEYS_REPETITIONS) {
        uint32_t receivedKeys = spiMaster->exchange(packet);
        processKeys(receivedKeys);
      } else {
        if (!reliablySend(packet, i))
          return false;
      }
    }

    return true;
  }

  bool sendSpatialDiffs(ImageDiffBitArray& diffs) {
    for (int i = 0; i < SPATIAL_DIFF_SIZE / PACKET_SIZE; i++) {
      uint32_t packet = ((uint32_t*)diffs.spatial)[i];
      if (!reliablySend(packet, i))
        return false;
    }

    return true;
  }

  bool sendPixels(Frame& frame, ImageDiffBitArray& diffs) {
    uint32_t compressedPixelId = 0;
    uint32_t outgoingPacket = 0;
    uint32_t sentPackets = 0;
    uint8_t byte = 0;

    for (int i = 0; i < frame.totalPixels; i++) {
      if (diffs.hasPixelChanged(i)) {
        // detect compression
        uint32_t block = compressedPixelId / SPATIAL_DIFF_BLOCK_SIZE;
        uint32_t blockPart = compressedPixelId % SPATIAL_DIFF_BLOCK_SIZE;
        compressedPixelId++;
        if (blockPart > 0 && diffs.isRepeatedBlock(block))
          continue;

        // send pixels
        outgoingPacket |= frame.raw8BitPixels[i] << (byte * 8);
        byte++;
        if (byte == PACKET_SIZE) {
          if (!reliablySend(outgoingPacket, sentPackets))
            return false;
          outgoingPacket = 0;
          byte = 0;
          sentPackets++;
        }
      }
    }

    if (byte > 0) {
      if (!reliablySend(outgoingPacket, sentPackets))
        return false;
    }

    return true;
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

  bool sync(uint32_t command, bool allowPause = false) {
    uint32_t local = command + CMD_RPI_OFFSET;
    uint32_t remote = command + CMD_GBA_OFFSET;

    return reliablySend(local, remote, allowPause);
  }

  bool reliablySend(uint32_t packetToSend,
                    uint32_t expectedResponse,
                    bool allowPause = true) {
    if (expectedResponse < MIN_COMMAND &&
        expectedResponse % TRANSFER_SYNC_FREQUENCY != 0) {
      spiMaster->send(packetToSend);
      return true;
    }

    uint32_t confirmation;
    uint32_t lastReceivedPacket = 0;

    while ((confirmation = spiMaster->exchange(packetToSend)) !=
           expectedResponse) {
      if (allowPause && confirmation == CMD_PAUSE + CMD_GBA_OFFSET) {
        if (!sync(CMD_PAUSE))
          return false;
        if (!sync(CMD_RESUME))
          return false;
      }

      if (confirmation == CMD_RESET) {
        LOG("Reset! (sent, expected, actual)");
        std::cout << "0x" << std::hex << packetToSend << "\n";
        std::cout << "0x" << std::hex << expectedResponse << "\n";
        std::cout << "0x" << std::hex << lastReceivedPacket << "\n\n";
        return false;
      }
      lastReceivedPacket = confirmation;
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
