#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include <fstream>
#include "BuildConfig.h"
#include "Config.h"
#include "Frame.h"
#include "FrameBuffer.h"
#include "ImageDiffRLECompressor.h"
#include "LoopbackAudio.h"
#include "PNGWriter.h"
#include "Palette.h"
#include "Protocol.h"
#include "ReliableStream.h"
#include "SPIMaster.h"
#include "Utils.h"
#include "VirtualGamepad.h"
using namespace std;

uint8_t LUT_24BPP_TO_8BIT_PALETTE[PALETTE_24BIT_MAX_COLORS];

uint32_t metadata;
uint32_t diffsStart;
uint32_t packetsToSend[MAX_PIXELS_SIZE];
uint32_t size;

class GBARemotePlay {
 public:
  GBARemotePlay() {
    config = new Config(CONFIG_FILENAME);
    spiMaster =
        new SPIMaster(SPI_MODE, config->spiSlowFrequency,
                      config->spiFastFrequency, config->spiDelayMicroseconds);
    reliableStream = new ReliableStream(spiMaster);
    frameBuffer = new FrameBuffer(DRAW_WIDTH, DRAW_HEIGHT);
    loopbackAudio = new LoopbackAudio();
    virtualGamepad = new VirtualGamepad(config->virtualGamepadName);
    lastFrame = Frame{0};

    PALETTE_initializeCache(PALETTE_CACHE_FILENAME);

    videoFile.open("video.bin", ios::out | ios::trunc | ios::binary);
    audioFile.open("audio.bin", ios::out | ios::trunc | ios::binary);
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

      ImageDiffRLECompressor diffs;
      diffs.initialize(frame, lastFrame, config->diffThreshold);

#ifdef PROFILE_VERBOSE
      auto frameDiffsElapsedTime = PROFILE_END(frameDiffsStartTime);
      auto frameTransferStartTime = PROFILE_START();
#endif

      // Don't send anything, just record the packets to a file
      // if (!send(frame, diffs)) {
      //   frame.clean();
      //   lastFrame.clean();
      //   goto reset;
      // }
      metadata = diffs.startPixel |
                 (diffs.expectedPackets() << PACKS_BIT_OFFSET) |
                 (diffs.shouldUseRLE() ? COMPR_BIT_MASK : 0) |
                 (frame.hasAudio() ? AUDIO_BIT_MASK : 0);
      diffsStart = (diffs.startPixel / 8) / PACKET_SIZE;
      size = 0;
      compressPixels(frame, diffs, packetsToSend, &size);

      // Clean previous frame
      lastFrame.clean();
      lastFrame = frame;

      // Record packets
      videoFile.write((char*)&metadata, PACKET_SIZE);
      videoFile.write(
          (char*)(((uint32_t*)(diffs.temporalDiffs)) + diffsStart),
          (TEMPORAL_DIFF_SIZE / PACKET_SIZE - diffsStart) * PACKET_SIZE);
      if (frame.hasAudio()) {
        audioFile.write((char*)frame.audioChunk,
                        AUDIO_SIZE_PACKETS * PACKET_SIZE);
      }
      videoFile.write((char*)packetsToSend, size * PACKET_SIZE);
      videoFile.flush();
      audioFile.flush();

#ifdef PROFILE_VERBOSE
      auto frameTransferElapsedTime = PROFILE_END(frameTransferStartTime);
      LOG("(build: " + std::to_string(frameGenerationElapsedTime) +
          "ms, diffs: " + std::to_string(frameDiffsElapsedTime) +
          "ms, transfer: " + std::to_string(frameTransferElapsedTime) + "ms)");
#endif

#ifdef PROFILE
      frames++;
      uint32_t elapsedTime = PROFILE_END(startTime);
      if (elapsedTime >= ONE_SECOND) {
        LOG("--- " + std::to_string(frames) + " frames ---");
        startTime = PROFILE_START();
        frames = 0;
      }
#endif
    }
  }

  ~GBARemotePlay() {
    lastFrame.clean();
    delete config;
    delete spiMaster;
    delete reliableStream;
    delete frameBuffer;
    delete loopbackAudio;
    delete virtualGamepad;
  }

 private:
  Config* config;
  SPIMaster* spiMaster;
  ReliableStream* reliableStream;
  FrameBuffer* frameBuffer;
  LoopbackAudio* loopbackAudio;
  VirtualGamepad* virtualGamepad;
  Frame lastFrame;
  uint32_t input;
  fstream videoFile;
  fstream audioFile;

  bool send(Frame& frame, ImageDiffRLECompressor& diffs) {
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
    LOG("  <" + std::to_string(idleElapsedTime) + "ms idle>");
    auto metadataStartTime = PROFILE_START();
#endif

    DEBULOG("Receiving keys and send metadata...");
    if (!receiveKeysAndSendMetadata(frame, diffs))
      return false;

#ifdef PROFILE_VERBOSE
    auto metadataElapsedTime = PROFILE_END(metadataStartTime);
    LOG("  <" + std::to_string(metadataElapsedTime) + "ms metadata>");
#endif

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

#ifdef DEBUG_PNG
    LOG("Writing debug PNG file...");
    WritePNG("debug.png", frame.raw8BitPixels, MAIN_PALETTE_24BPP, RENDER_WIDTH,
             RENDER_HEIGHT);
    LOG("Frame end!");
#endif

    return true;
  }

  bool receiveKeysAndSendMetadata(Frame& frame, ImageDiffRLECompressor& diffs) {
  again:
    metadata = diffs.startPixel |
               (diffs.expectedPackets() << PACKS_BIT_OFFSET) |
               (diffs.shouldUseRLE() ? COMPR_BIT_MASK : 0) |
               (frame.hasAudio() ? AUDIO_BIT_MASK : 0);
    uint32_t keys = spiMaster->exchange(metadata);

    if (reliableStream->finishSyncIfNeeded(keys, CMD_FRAME_START))
      goto again;
    if (spiMaster->exchange(keys) != metadata)
      return false;
    processKeys(keys);

    diffsStart = (diffs.startPixel / 8) / PACKET_SIZE;

    return reliableStream->send(diffs.temporalDiffs,
                                TEMPORAL_DIFF_SIZE / PACKET_SIZE,
                                CMD_FRAME_START, diffsStart);
  }

  bool sendAudio(Frame& frame) {
    return reliableStream->send(frame.audioChunk, AUDIO_SIZE_PACKETS,
                                CMD_AUDIO);
  }

  bool compressAndSendPixels(Frame& frame, ImageDiffRLECompressor& diffs) {
    size = 0;
    compressPixels(frame, diffs, packetsToSend, &size);

#ifdef DEBUG
    if (size != diffs.expectedPackets()) {
      LOG("[!!!] Sizes don't match (" + std::to_string(size) + " vs " +
          std::to_string(diffs.expectedPackets()) + ")");
    }
#endif

#ifdef PROFILE_VERBOSE
    LOG("  <" + std::to_string(size * PACKET_SIZE) + "bytes" +
        (diffs.shouldUseRLE()
             ? ", rle (" + std::to_string(diffs.omittedRLEPixels()) +
                   " omitted)"
             : "") +
        (frame.hasAudio() ? ", audio>" : ">"));
#endif

    return reliableStream->send(packetsToSend, size, CMD_PIXELS);
  }

  void compressPixels(Frame& frame,
                      ImageDiffRLECompressor& diffs,
                      uint32_t* packets,
                      uint32_t* totalPackets) {
    uint32_t currentPacket = 0;
    uint8_t byte = 0;

#define ADD_BYTE(DATA)                      \
  currentPacket |= DATA << (byte * 8);      \
  byte++;                                   \
  if (byte == PACKET_SIZE) {                \
    packets[*totalPackets] = currentPacket; \
    currentPacket = 0;                      \
    byte = 0;                               \
    (*totalPackets)++;                      \
  }

    if (diffs.shouldUseRLE()) {
      uint32_t rleIndex = 0, pixelIndex = 0;

      while (rleIndex < diffs.totalEncodedPixels()) {
        uint8_t times = diffs.runLengthEncoding[rleIndex];
        uint8_t pixel = diffs.compressedPixels[pixelIndex];

        ADD_BYTE(times)
        ADD_BYTE(pixel)

        pixelIndex += times;
        rleIndex++;
      }
    } else {
      for (int i = 0; i < diffs.totalCompressedPixels; i++) {
        uint8_t pixel = diffs.compressedPixels[i];
        ADD_BYTE(pixel)
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

    // Sleep to limit frames per second
    usleep(config->recordSleepMilliseconds * 1000);

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
