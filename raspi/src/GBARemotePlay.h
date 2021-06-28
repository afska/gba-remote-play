#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include "Config.h"
#include "Frame.h"
#include "FrameBuffer.h"
#include "ImageDiffBitArray.h"
#include "LoopbackAudio.h"
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
    frameBuffer = new FrameBuffer(DRAW_WIDTH, DRAW_HEIGHT);
    loopbackAudio = new LoopbackAudio();
    virtualGamepad = new VirtualGamepad(VIRTUAL_GAMEPAD_NAME);
    lastFrame = Frame{0};

    PALETTE_initializeCache(PALETTE_CACHE_FILENAME);
  }

  void run() {
#ifdef PROFILE
    auto startTime = PROFILE_START();
    uint32_t frames = 0;
#endif

  reset:
    spiMaster->send(CMD_RESET);

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
    delete reliableStream;
    delete frameBuffer;
    delete loopbackAudio;
    delete virtualGamepad;
  }

 private:
  SPIMaster* spiMaster;
  ReliableStream* reliableStream;
  FrameBuffer* frameBuffer;
  LoopbackAudio* loopbackAudio;
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

    DEBULOG("Receiving keys and send metadata...");
    if (!receiveKeysAndSendMetadata(frame, diffs))
      return false;

    if (frame.hasAudio()) {
      DEBULOG("Syncing audio...");
      if (!reliableStream->sync(CMD_AUDIO))
        return false;

      DEBULOG("Sending audio...");
      if (!sendAudio(frame))
        return false;
    }

    DEBULOG("Syncing pixels...");
    if (!reliableStream->sync(CMD_PIXELS))
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

  bool receiveKeysAndSendMetadata(Frame& frame, ImageDiffBitArray& diffs) {
  again:
    uint32_t expectedPackets = (diffs.compressedPixels / PIXELS_PER_PACKET +
                                diffs.compressedPixels % PIXELS_PER_PACKET) |
                               (frame.hasAudio() ? AUDIO_BIT_MASK : 0);
    uint32_t keys = spiMaster->exchange(expectedPackets);
    if (reliableStream->finishSyncIfNeeded(keys, CMD_FRAME_START))
      goto again;
    if (spiMaster->exchange(keys) != expectedPackets)
      return false;
    processKeys(keys);

    return reliableStream->send(
        diffs.temporal, TEMPORAL_DIFF_SIZE / PACKET_SIZE, CMD_FRAME_START);
  }

  bool sendAudio(Frame& frame) {
    return reliableStream->send(frame.audioChunk, AUDIO_SIZE_PACKETS,
                                CMD_AUDIO);
  }

  bool compressAndSendPixels(Frame& frame, ImageDiffBitArray& diffs) {
    uint32_t packetsToSend[MAX_PIXELS_SIZE];
    uint32_t size = 0;
    compressPixels(frame, diffs, packetsToSend, &size);

    return size > MID_FRAME_AUDIO_PERIOD
               ? sendImageWithMidFrameAudio(packetsToSend, size)
               : sendImageOnly(packetsToSend, size);
  }

  bool sendImageOnly(uint32_t* packetsToSend, uint32_t size) {
    return reliableStream->send(packetsToSend, size, CMD_PIXELS);
  }

  bool sendImageWithMidFrameAudio(uint32_t* packetsToSend, uint32_t size) {
    // (first part of the pixels)
    reliableStream->send(packetsToSend, MID_FRAME_AUDIO_PERIOD, CMD_PIXELS);

    // (mid-frame audio load)
    auto audioChunk = loopbackAudio->loadChunk();
    bool hasAudio = audioChunk != NULL;
    if (!reliableStream->sendValue(hasAudio, MID_FRAME_AUDIO_PERIOD,
                                   CMD_PIXELS)) {
      free(audioChunk);
      return false;
    }

    // (mid-frame audio transfer)
    if (hasAudio) {
      bool result =
          reliableStream->send(audioChunk, AUDIO_SIZE_PACKETS, CMD_PIXELS);
      free(audioChunk);
      if (!result)
        return false;
    }

    // (second part of the pixels)
    return reliableStream->send(packetsToSend, size, CMD_PIXELS,
                                MID_FRAME_AUDIO_PERIOD);
  }

  void compressPixels(Frame& frame,
                      ImageDiffBitArray& diffs,
                      uint32_t* packets,
                      uint32_t* totalPackets) {
    uint32_t currentPacket = 0;
    uint8_t byte = 0;

    for (int i = 0; i < frame.totalPixels; i++) {
      if (diffs.hasPixelChanged(i)) {
        currentPacket |= frame.raw8BitPixels[i] << (byte * 8);
        byte++;
        if (byte == PACKET_SIZE) {
          packets[*totalPackets] = currentPacket;
          currentPacket = 0;
          byte = 0;
          (*totalPackets)++;
        }
      }
    }

    if (byte > 0) {
      packets[*totalPackets] = currentPacket;
      (*totalPackets)++;
    }
  }

  Frame loadFrame() {
    Frame frame;
    frame.totalPixels = TOTAL_PIXELS;
    frame.raw8BitPixels = (uint8_t*)malloc(TOTAL_PIXELS);
    frame.palette = MAIN_PALETTE_24BPP;

    frameBuffer->forEachPixel(
        [&frame, this](int x, int y, uint8_t r, uint8_t g, uint8_t b) {
          if (x % DRAW_SCALE_X != 0 || y % DRAW_SCALE_Y != 0)
            return;
          x = x / DRAW_SCALE_X;
          y = y / DRAW_SCALE_Y;

          frame.raw8BitPixels[y * RENDER_WIDTH + x] =
              LUT_24BPP_TO_8BIT_PALETTE[(r << 0) | (g << 8) | (b << 16)];
        });

    frame.audioChunk = loopbackAudio->loadChunk();

    return frame;
  }

  void processKeys(uint16_t keys) { virtualGamepad->setKeys(keys); }
};

#endif  // GBA_REMOTE_PLAY_H
