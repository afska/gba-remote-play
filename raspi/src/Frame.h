#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdlib.h>
#include "Protocol.h"

typedef struct Frame {
  uint32_t totalPixels;
  uint8_t* raw8BitPixels;
  const uint32_t* palette;
  uint8_t* audioChunk;

  uint32_t getColorOf(uint32_t pixelId) {
    return palette[raw8BitPixels[pixelId]];
  }

  bool hasPixelChanged(uint32_t pixelId, Frame previousFrame) {
    if (!previousFrame.hasData())
      return true;

    return arePixelsDifferent(&previousFrame, this, pixelId, pixelId);
  }

  bool hasData() { return totalPixels > 0; }

  void clean() {
    if (!hasData())
      return;

    totalPixels = 0;
    free(raw8BitPixels);
    free(audioChunk);
  }

  bool arePixelsDifferent(Frame* oldPixelFrame,
                          Frame* newPixelFrame,
                          uint32_t oldPixelId,
                          uint32_t newPixelId) {
    uint32_t color1 = oldPixelFrame->getColorOf(oldPixelId);
    uint32_t color2 = newPixelFrame->getColorOf(newPixelId);

    int r1 = (color1 >> 0) & 0xff;
    int g1 = (color1 >> 8) & 0xff;
    int b1 = (color1 >> 16) & 0xff;

    int r2 = (color2 >> 0) & 0xff;
    int g2 = (color2 >> 8) & 0xff;
    int b2 = (color2 >> 16) & 0xff;

    // TODO: REMOVE DUPLICATED CODE (PALETTE_getClosestColor)
    int diffR = r1 - r2;
    int diffG = g1 - g2;
    int diffB = b1 - b2;
    int distanceSquared = diffR * diffR + diffG * diffG + diffB * diffB;
    bool areDifferent = distanceSquared > TEMPORAL_DIFF_THRESHOLD;

    if (!areDifferent)
      newPixelFrame->raw8BitPixels[newPixelId] =
          oldPixelFrame->raw8BitPixels[oldPixelId];

    return areDifferent;
  }
} Frame;

#endif  // FRAME_H
