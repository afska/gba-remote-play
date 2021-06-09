#ifndef PALETTE_H
#define PALETTE_H

#include <stdint.h>
#include <stdio.h>
#include <fstream>
#include <iostream>

#define PALETTE_24BIT_MAX_COLORS 16777216

extern uint8_t LUT_24BPP_TO_8BIT_PALETTE[PALETTE_24BIT_MAX_COLORS];

const uint32_t MAIN_PALETTE_24BPP[] = {
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

inline uint8_t PALETTE_getClosestColor(uint8_t r, uint8_t g, uint8_t b) {
  uint32_t minDistanceSquared = 0xff * 0xff + 0xff * 0xff + 0xff * 0xff + 1;
  uint8_t bestColorIndex = 0;

  for (int i = 0; i < 256; i++) {
    int diffR = r - (MAIN_PALETTE_24BPP[i] >> 0) & 0xff;
    int diffG = g - (MAIN_PALETTE_24BPP[i] >> 8) & 0xff;
    int diffB = b - (MAIN_PALETTE_24BPP[i] >> 16) & 0xff;
    int distanceSquared = diffR * diffR + diffG * diffG + diffB * diffB;

    if (distanceSquared < minDistanceSquared) {
      minDistanceSquared = distanceSquared;
      bestColorIndex = i;
    }
  }

  return bestColorIndex == 0 ? 255 : bestColorIndex;
}

inline void PALETTE_initializeCache(std::string fileName) {
  FILE* file = fopen(fileName.c_str(), "r");

  if (file != NULL) {
    std::cout << "Loading cache... <= " + fileName + "\n";
    fread(LUT_24BPP_TO_8BIT_PALETTE, 1, PALETTE_24BIT_MAX_COLORS, file);
    std::cout << "Cache loaded!\n\n";
    fclose(file);
  } else {
    for (int i = 0; i < PALETTE_24BIT_MAX_COLORS; i++) {
      uint8_t r = (i >> 0) & 0xff;
      uint8_t g = (i >> 8) & 0xff;
      uint8_t b = (i >> 16) & 0xff;
      LUT_24BPP_TO_8BIT_PALETTE[i] = PALETTE_getClosestColor(r, g, b);

      if (i % 1000 == 0)
        std::cout << "Creating cache (" + std::to_string(i) + "/" +
                         std::to_string(PALETTE_24BIT_MAX_COLORS) + ")...\n";
    }

    FILE* newFile = fopen(fileName.c_str(), "w+");
    fwrite(LUT_24BPP_TO_8BIT_PALETTE, 1, PALETTE_24BIT_MAX_COLORS, newFile);
    fclose(newFile);

    std::cout << "Cache saved => " + fileName + "\n\n";
  }
}

/*
// 6-8-5 levels RGB

This palette is constructed with:
  - six levels for red
  - eight levels for green
  - five levels for the blue primaries
...giving 6×8×5 = 240 combinations
Levels are chosen in function of sensibility of the normal human eye to every
primary color. Also, it does not provide true grays. Remaining indexes are
filled with intermediate grays.

--------
JS Code:
--------

const cartesian =
  (...a) => a.reduce((a, b) => a.flatMap(d => b.map(e => [d, e].flat())));

const rgb24To15bpp = (c) => c * 32 / 256;
const build24bpp = (r, g, b) => (r << 0) | (g << 8) | (b << 16)
const build15bpp = (r, g, b) =>
  rgb24To15bpp(r) | (rgb24To15bpp(g) << 5) | (rgb24To15bpp(b) << 10);

const colors = cartesian(
  [0x00, 0x33, 0x66, 0x99, 0xcc, 0xff],
  [0x00, 0x24, 0x49, 0x6d, 0x92, 0xb6, 0xdb, 0xff],
  [0x00, 0x40, 0x80, 0xbf, 0xff]
).map(([r, g, b]) => build15bpp(r, g, b));

const shadesOfGray = [
  0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee
].map((c) => build15bpp(c, c, c));

const palette = colors.concat(shadesOfGray).concat([0, 0]);
// colors (240) + shades of gray (14) + padding (2)
*/

#endif  // PALETTE_H
