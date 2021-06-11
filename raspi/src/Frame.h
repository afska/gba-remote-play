#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdlib.h>
#include "Protocol.h"

typedef struct Frame {
  uint32_t totalPixels;
  uint8_t* raw8BitPixels;
  const uint32_t* palette;

  uint32_t getColorOf(uint32_t pixelId) {
    return palette[raw8BitPixels[pixelId]];
  }

  bool hasPixelChanged(uint32_t pixelId, Frame previousFrame) {
    if (!previousFrame.hasData())
      return true;

    bool hasChanged =
        areDifferent(getColorOf(pixelId), previousFrame.getColorOf(pixelId));
    if (!hasChanged)
      raw8BitPixels[pixelId] = previousFrame.raw8BitPixels[pixelId];

    return hasChanged;
  }

  bool hasData() { return totalPixels > 0; }

  void clean() {
    if (!hasData())
      return;

    totalPixels = 0;
    free(raw8BitPixels);
  }

  bool areDifferent(uint32_t color1, uint32_t color2) {
    int r1 = (color1 >> 0) & 0xff;
    int g1 = (color1 >> 8) & 0xff;
    int b1 = (color1 >> 16) & 0xff;

    int r2 = (color2 >> 0) & 0xff;
    int g2 = (color2 >> 8) & 0xff;
    int b2 = (color2 >> 16) & 0xff;

    int diffR = r1 - r2;
    int diffG = g1 - g2;
    int diffB = b1 - b2;
    int distanceSquared = diffR * diffR + diffG * diffG + diffB * diffB;

    return distanceSquared > DIFF_THRESHOLD;
  }
} Frame;

#endif  // FRAME_H
