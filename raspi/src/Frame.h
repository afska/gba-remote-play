#ifndef FRAME_H
#define FRAME_H

#include <stdint.h>
#include <stdlib.h>

typedef struct Frame {
  uint32_t totalPixels;
  uint8_t* raw8BitPixels;

  uint8_t getColorIndexOf(uint32_t pixelId) { return raw8BitPixels[pixelId]; }

  bool hasPixelChanged(uint32_t pixelId, Frame previousFrame) {
    return !previousFrame.hasData() ||
           getColorIndexOf(pixelId) != previousFrame.getColorIndexOf(pixelId);
  }

  bool hasData() { return totalPixels > 0; }

  void clean() {
    if (!hasData())
      return;

    totalPixels = 0;
    free(raw8BitPixels);
  }
} Frame;

#endif  // FRAME_H
