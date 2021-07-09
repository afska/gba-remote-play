#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdlib.h>
#include "Protocol.h"
#include "Utils.h"

typedef struct Frame {
  uint32_t totalPixels;
  uint8_t* raw8BitPixels;
  const uint32_t* palette;
  uint8_t* audioChunk;

  uint32_t getColorOf(uint32_t pixelId) {
    return palette[raw8BitPixels[pixelId]];
  }

  bool hasPixelChanged(uint32_t pixelId,
                       Frame previousFrame,
                       uint32_t threshold = 0) {
    if (!previousFrame.hasData())
      return true;

    return arePixelsDifferent(&previousFrame, this, pixelId, pixelId,
                              threshold);
  }

  bool hasData() { return totalPixels > 0; }
  bool hasAudio() { return audioChunk != NULL; }

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
                          uint32_t newPixelId,
                          uint32_t threshold) {
    uint32_t color1 = oldPixelFrame->getColorOf(oldPixelId);
    uint32_t color2 = newPixelFrame->getColorOf(newPixelId);

    int r1 = (color1 >> 0) & 0xff;
    int g1 = (color1 >> 8) & 0xff;
    int b1 = (color1 >> 16) & 0xff;
    int distanceSquared = getDistanceSquared(r1, g1, b1, color2);
    bool areDifferent = distanceSquared > threshold;

    if (!areDifferent)
      newPixelFrame->raw8BitPixels[newPixelId] =
          oldPixelFrame->raw8BitPixels[oldPixelId];

    return areDifferent;
  }
} Frame;

#endif  // FRAME_H
