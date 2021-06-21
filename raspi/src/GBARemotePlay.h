#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include "Config.h"
#include "Frame.h"
#include "FrameBuffer.h"
#include "ImageDiffBitArray.h"
#include "PNGWriter.h"
#include "Palette.h"
#include "Protocol.h"
#include "ReliableStream.h"
#include "SPIMaster.h"
#include "VirtualGamepad.h"

uint8_t LUT_24BPP_TO_8BIT_PALETTE[PALETTE_24BIT_MAX_COLORS];

class GBARemotePlay {
 public:
  GBARemotePlay() {
    spiMaster = new SPIMaster(SPI_MODE, SPI_SLOW_FREQUENCY, SPI_FAST_FREQUENCY,
                              SPI_DELAY_MICROSECONDS);
    reliableStream = new ReliableStream(spiMaster);
    frameBuffer = new FrameBuffer(RENDER_WIDTH, RENDER_HEIGHT);
    virtualGamepad = new VirtualGamepad(VIRTUAL_GAMEPAD_NAME);
    lastFrame = Frame{0};

    PALETTE_initializeCache(PALETTE_CACHE_FILENAME);
  }

  void run() {
  reset:
    lastFrame.clean();
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
        LOG("Reset!\n");
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
    delete reliableStream;
    delete frameBuffer;
    delete virtualGamepad;
  }

 private:
  SPIMaster* spiMaster;
  ReliableStream* reliableStream;
  FrameBuffer* frameBuffer;
  VirtualGamepad* virtualGamepad;
  Frame lastFrame;
  uint32_t input;

  bool send(Frame& frame, ImageDiffBitArray& diffs) {
    if (!frame.hasData())
      return false;

#ifdef PROFILE_VERBOSE
    auto idleStartTime = PROFILE_START();
#endif

    DEBULOG("Syncing frame start...");
    if (!reliableStream->sync(CMD_FRAME_START))
      return false;

#ifdef PROFILE_VERBOSE
    auto idleElapsedTime = PROFILE_END(idleStartTime);
    std::cout << "  <" + std::to_string(idleElapsedTime) + "ms idle>\n";
#endif

    DEBULOG("Receiving keys and sending temporal diffs...");
    if (!receiveKeysAndSendTemporalDiffs(diffs))
      return false;

    DEBULOG("Syncing spatial diffs start...");
    if (!reliableStream->sync(CMD_SPATIAL_DIFFS_START))
      return false;

    DEBULOG("Sending spatial diffs...");
    if (!sendSpatialDiffs(diffs))
      return false;

    DEBULOG("Syncing pixels start...");
    if (!reliableStream->sync(CMD_PIXELS_START))
      return false;

    DEBULOG("Sending pixels...");
    if (!compressAndSendPixels(frame, diffs))
      return false;

    DEBULOG("Syncing frame end...");
    if (!reliableStream->sync(CMD_FRAME_END))
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
    uint32_t expectedPackets = diffs.compressedPixels / PIXELS_PER_PACKET +
                               diffs.compressedPixels % PIXELS_PER_PACKET;
    uint32_t keys = spiMaster->exchange(expectedPackets);
    for (int i = 0; i < SYNC_VALIDATIONS; i++)
      if (spiMaster->exchange(expectedPackets) != keys)
        return false;

    processKeys(keys);

    uint32_t i = 0;
    while (i < TEMPORAL_DIFF_SIZE / PACKET_SIZE) {
      uint32_t packetToSend = ((uint32_t*)diffs.temporal)[i];
      if (!reliableStream->send(packetToSend, &i))
        return false;
    }

    return true;
  }

  bool sendSpatialDiffs(ImageDiffBitArray& diffs) {
    uint32_t i = 0;
    while (i < SPATIAL_DIFF_SIZE / PACKET_SIZE) {
      uint32_t packetToSend = ((uint32_t*)diffs.spatial)[i];
      if (!reliableStream->send(packetToSend, &i))
        return false;
    }

    return true;
  }

  bool compressAndSendPixels(Frame& frame, ImageDiffBitArray& diffs) {
    uint32_t packetsToSend[MAX_PIXELS_SIZE];
    uint32_t totalPackets = 0;
    compressPixels(frame, diffs, packetsToSend, &totalPackets);

    uint32_t i = 0;
    while (i < totalPackets) {
      uint32_t packetToSend = packetsToSend[i];
      if (!reliableStream->send(packetToSend, &i))
        return false;
    }
    // TODO: SAME STEP, MEASURE COMPRESSION TIME

    return true;
  }

  void compressPixels(Frame& frame,
                      ImageDiffBitArray& diffs,
                      uint32_t* packetsToSend,
                      uint32_t* totalPackets) {
    uint32_t compressedPixelId = 0;
    uint32_t packetToSend = 0;
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
        packetToSend |= frame.raw8BitPixels[i] << (byte * 8);
        byte++;
        if (byte == PACKET_SIZE) {
          packetsToSend[*totalPackets] = packetToSend;
          packetToSend = 0;
          byte = 0;
          (*totalPackets)++;
        }
      }
    }

    if (byte > 0) {
      packetsToSend[*totalPackets] = packetToSend;
      (*totalPackets)++;
    }
  }

  void processKeys(uint16_t keys) { virtualGamepad->setKeys(keys); }

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
