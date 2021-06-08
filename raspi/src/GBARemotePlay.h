#ifndef GBA_REMOTE_PLAY_H
#define GBA_REMOTE_PLAY_H

#include <stdlib.h>  // TODO: MOVE
#include "Config.h"
#include "FrameBuffer.h"
#include "ImageQuantizer.h"
#include "PNGWriter.h"
#include "Protocol.h"
#include "SPIMaster.h"
#include "TemporalDiffBitArray.h"
#include "VirtualGamepad.h"

uint32_t MAIN_PALETTE[] = {
    0,        4194304,  8388608,  12517376, 16711680, 9216,     4203520,
    8397824,  12526592, 16720896, 18688,    4212992,  8407296,  12536064,
    16730368, 27904,    4222208,  8416512,  12545280, 16739584, 37376,
    4231680,  8425984,  12554752, 16749056, 46592,    4240896,  8435200,
    12563968, 16758272, 56064,    4250368,  8444672,  12573440, 16767744,
    65280,    4259584,  8453888,  12582656, 16776960, 51,       4194355,
    8388659,  12517427, 16711731, 9267,     4203571,  8397875,  12526643,
    16720947, 18739,    4213043,  8407347,  12536115, 16730419, 27955,
    4222259,  8416563,  12545331, 16739635, 37427,    4231731,  8426035,
    12554803, 16749107, 46643,    4240947,  8435251,  12564019, 16758323,
    56115,    4250419,  8444723,  12573491, 16767795, 65331,    4259635,
    8453939,  12582707, 16777011, 102,      4194406,  8388710,  12517478,
    16711782, 9318,     4203622,  8397926,  12526694, 16720998, 18790,
    4213094,  8407398,  12536166, 16730470, 28006,    4222310,  8416614,
    12545382, 16739686, 37478,    4231782,  8426086,  12554854, 16749158,
    46694,    4240998,  8435302,  12564070, 16758374, 56166,    4250470,
    8444774,  12573542, 16767846, 65382,    4259686,  8453990,  12582758,
    16777062, 153,      4194457,  8388761,  12517529, 16711833, 9369,
    4203673,  8397977,  12526745, 16721049, 18841,    4213145,  8407449,
    12536217, 16730521, 28057,    4222361,  8416665,  12545433, 16739737,
    37529,    4231833,  8426137,  12554905, 16749209, 46745,    4241049,
    8435353,  12564121, 16758425, 56217,    4250521,  8444825,  12573593,
    16767897, 65433,    4259737,  8454041,  12582809, 16777113, 204,
    4194508,  8388812,  12517580, 16711884, 9420,     4203724,  8398028,
    12526796, 16721100, 18892,    4213196,  8407500,  12536268, 16730572,
    28108,    4222412,  8416716,  12545484, 16739788, 37580,    4231884,
    8426188,  12554956, 16749260, 46796,    4241100,  8435404,  12564172,
    16758476, 56268,    4250572,  8444876,  12573644, 16767948, 65484,
    4259788,  8454092,  12582860, 16777164, 255,      4194559,  8388863,
    12517631, 16711935, 9471,     4203775,  8398079,  12526847, 16721151,
    18943,    4213247,  8407551,  12536319, 16730623, 28159,    4222463,
    8416767,  12545535, 16739839, 37631,    4231935,  8426239,  12555007,
    16749311, 46847,    4241151,  8435455,  12564223, 16758527, 56319,
    4250623,  8444927,  12573695, 16767999, 65535,    4259839,  8454143,
    12582911, 16777215, 1118481,  2236962,  3355443,  4473924,  5592405,
    6710886,  7829367,  8947848,  10066329, 11184810, 12303291, 13421772,
    14540253, 15658734, 0,        0};

const uint16_t MAIN_PALETTE_15BPP[] = {
    0,     8192,  16384, 23552, 31744, 128,   8320,  16512, 23680, 31872, 288,
    8480,  16672, 23840, 32032, 416,   8608,  16800, 23968, 32160, 576,   8768,
    16960, 24128, 32320, 704,   8896,  17088, 24256, 32448, 864,   9056,  17248,
    24416, 32608, 992,   9184,  17376, 24544, 32736, 6,     8198,  16390, 23558,
    31750, 134,   8326,  16518, 23686, 31878, 294,   8486,  16678, 23846, 32038,
    422,   8614,  16806, 23974, 32166, 582,   8774,  16966, 24134, 32326, 710,
    8902,  17094, 24262, 32454, 870,   9062,  17254, 24422, 32614, 998,   9190,
    17382, 24550, 32742, 12,    8204,  16396, 23564, 31756, 140,   8332,  16524,
    23692, 31884, 300,   8492,  16684, 23852, 32044, 428,   8620,  16812, 23980,
    32172, 588,   8780,  16972, 24140, 32332, 716,   8908,  17100, 24268, 32460,
    876,   9068,  17260, 24428, 32620, 1004,  9196,  17388, 24556, 32748, 19,
    8211,  16403, 23571, 31763, 147,   8339,  16531, 23699, 31891, 307,   8499,
    16691, 23859, 32051, 435,   8627,  16819, 23987, 32179, 595,   8787,  16979,
    24147, 32339, 723,   8915,  17107, 24275, 32467, 883,   9075,  17267, 24435,
    32627, 1011,  9203,  17395, 24563, 32755, 25,    8217,  16409, 23577, 31769,
    153,   8345,  16537, 23705, 31897, 313,   8505,  16697, 23865, 32057, 441,
    8633,  16825, 23993, 32185, 601,   8793,  16985, 24153, 32345, 729,   8921,
    17113, 24281, 32473, 889,   9081,  17273, 24441, 32633, 1017,  9209,  17401,
    24569, 32761, 31,    8223,  16415, 23583, 31775, 159,   8351,  16543, 23711,
    31903, 319,   8511,  16703, 23871, 32063, 447,   8639,  16831, 23999, 32191,
    607,   8799,  16991, 24159, 32351, 735,   8927,  17119, 24287, 32479, 895,
    9087,  17279, 24447, 32639, 1023,  9215,  17407, 24575, 32767, 2114,  4228,
    6342,  8456,  10570, 12684, 14798, 17969, 20083, 22197, 24311, 26425, 28539,
    30653, 0,     0};

class GBARemotePlay {
 public:
  GBARemotePlay() {
    spiMaster = new SPIMaster(SPI_MODE, SPI_SLOW_FREQUENCY, SPI_FAST_FREQUENCY,
                              SPI_DELAY_MICROSECONDS);
    frameBuffer = new FrameBuffer(RENDER_WIDTH, RENDER_HEIGHT);
    imageQuantizer = new ImageQuantizer();
    virtualGamepad = new VirtualGamepad(VIRTUAL_GAMEPAD_NAME);
    lastFrame = Frame{0};
    resetKeys();
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
      // auto frame = imageQuantizer->quantize((uint8_t*)rgbaPixels,
      // RENDER_WIDTH,
      //                                       RENDER_HEIGHT, QUANTIZER_SPEED);
      TemporalDiffBitArray diffs;
      diffs.initialize(frame, lastFrame);

#ifdef PROFILE_VERBOSE
      auto frameGenerationElapsedTime = PROFILE_END(frameGenerationStartTime);
      auto frameTransferStartTime = PROFILE_START();
#endif

      if (!send(frame, diffs))
        goto reset;

      lastFrame.clean();
      lastFrame = frame;

#ifdef PROFILE_VERBOSE
      auto frameTransferElapsedTime = PROFILE_END(frameTransferStartTime);
      std::cout << "(build: " + std::to_string(frameGenerationElapsedTime) +
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
    delete imageQuantizer;
    delete virtualGamepad;
  }

 private:
  SPIMaster* spiMaster;
  FrameBuffer* frameBuffer;
  ImageQuantizer* imageQuantizer;
  VirtualGamepad* virtualGamepad;
  Frame lastFrame;
  uint32_t input;
  uint32_t inputValidations;

  bool send(Frame& frame, TemporalDiffBitArray& diffs) {
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

    DEBULOG("Sending palette command...");
    if (!sync(CMD_PALETTE_START_RPI, CMD_PALETTE_START_GBA))
      return false;

    DEBULOG("Sending palette...");
    sendPalette(frame);

    DEBULOG("Sending pixels command...");
    if (!sync(CMD_PIXELS_START_RPI, CMD_PIXELS_START_GBA))
      return false;

    DEBULOG("Sending pixels...");
    sendPixels(frame);

    DEBULOG("Sending end command...");
    if (!sync(CMD_FRAME_END_RPI, CMD_FRAME_END_GBA))
      return false;

#ifdef DEBUG
    LOG("Writing debug PNG file...");
    WritePNG("debug.png", frame.raw8BitPixels, frame.raw15bppPalette,
             RENDER_WIDTH, RENDER_HEIGHT);
    LOG("Frame end!");
#endif

    return true;
  }

  void sendDiffs(TemporalDiffBitArray& diffs) {
    for (int i = 0; i < TEMPORAL_DIFF_SIZE / PACKET_SIZE; i++) {
      uint32_t packet = ((uint32_t*)diffs.data)[i];

      if (i < PRESSED_KEYS_REPETITIONS) {
        uint32_t receivedKeys = spiMaster->exchange(packet);
        processKeys(receivedKeys);
      } else
        spiMaster->send(packet);
    }

    spiMaster->send(diffs.expectedPixels);
  }

  void sendPalette(Frame& frame) {
    for (int i = 0; i < PALETTE_COLORS / COLORS_PER_PACKET; i++) {
      uint32_t packet = ((uint32_t*)frame.raw15bppPalette)[i];

      if (i < PRESSED_KEYS_REPETITIONS) {
        uint32_t receivedKeys = spiMaster->exchange(packet);
        processKeys(receivedKeys);
      } else
        spiMaster->send(packet);
    }
  }

  void sendPixels(Frame& frame) {
    uint32_t outgoingPacket = 0;
    uint8_t byte = 0;
    for (int i = 0; i < frame.totalPixels; i++) {
      if (frame.hasPixelChanged(i, lastFrame)) {
        outgoingPacket |= frame.raw8BitPixels[i] << (byte * 8);

        byte++;
        if (byte == PACKET_SIZE || i == frame.totalPixels - 1) {
          spiMaster->send(outgoingPacket);
          outgoingPacket = 0;
          byte = 0;
        }
      }
    }
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
    frame.raw15bppPalette = (uint16_t*)malloc(PALETTE_COLORS * 2);
    frame.raw8BitPixels = (uint8_t*)malloc(TOTAL_PIXELS * 1);
    memcpy(frame.raw15bppPalette, MAIN_PALETTE_15BPP, PALETTE_COLORS * 2);

    frameBuffer->forEachPixel([&frame](int x, int y, uint8_t r, uint8_t g,
                                       uint8_t b) {
      uint32_t minDistanceSquared = 0xff * 0xff + 0xff * 0xff + 0xff * 0xff + 1;
      uint8_t bestColorIndex = 0;

      for (int i = 0; i < PALETTE_COLORS; i++) {
        int diffR = r - MAIN_PALETTE[i] & 0xff;
        int diffG = g - (MAIN_PALETTE[i] >> 8) & 0xff;
        int diffB = b - (MAIN_PALETTE[i] >> 16) & 0xff;
        int distanceSquared = diffR * diffR + diffG * diffG + diffB * diffB;

        if (distanceSquared < minDistanceSquared) {
          minDistanceSquared = distanceSquared;
          bestColorIndex = i;
        }
      }

      frame.raw8BitPixels[y * RENDER_WIDTH + x] = bestColorIndex;
    });

    return frame;
  }
};

#endif  // GBA_REMOTE_PLAY_H
