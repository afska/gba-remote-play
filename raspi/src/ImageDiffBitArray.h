#ifndef IMAGE_DIFF_BIT_ARRAY_H
#define IMAGE_DIFF_BIT_ARRAY_H

#include <stdint.h>
#include "Frame.h"
#include "Protocol.h"

typedef struct {
  uint8_t temporal[TEMPORAL_DIFF_SIZE];
  uint8_t spatial[SPATIAL_DIFF_MAX_SIZE];
  uint32_t expectedPixels;

  void initialize(Frame currentFrame, Frame previousFrame) {
    expectedPixels = 0;
    bool hasDifferentColors = false;

    for (int i = 0; i < TOTAL_PIXELS; i++) {
      uint8_t byte = expectedPixels % PIXELS_PER_PACKET;

      if (currentFrame.hasPixelChanged(i, previousFrame)) {
        if (byte > 0) {
          hasDifferentColors =
              hasDifferentColors ||
              currentFrame.arePixelsDifferent(&currentFrame, &currentFrame,
                                              i - byte, i, false);
        } else if (expectedPixels > 0) {
          setBit(spatial, expectedPixels / PIXELS_PER_PACKET,
                 hasDifferentColors);
          hasDifferentColors = false;
        }

        setBit(temporal, i, true);
        expectedPixels++;
      } else
        setBit(temporal, i, false);
    }
  }

 private:
  void setBit(uint8_t* bitarray, uint32_t n, bool value) {
    uint32_t byte = n / 8;
    uint8_t bit = n % 8;

    bitarray[byte] =
        value ? bitarray[byte] | (1 << bit) : bitarray[byte] & ~(1 << bit);
  }
} ImageDiffBitArray;

#endif  // IMAGE_DIFF_BIT_ARRAY_H
