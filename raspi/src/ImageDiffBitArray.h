#ifndef IMAGE_DIFF_BIT_ARRAY_H
#define IMAGE_DIFF_BIT_ARRAY_H

#include <stdint.h>
#include "Frame.h"
#include "Protocol.h"

typedef struct {
  uint8_t temporal[TEMPORAL_DIFF_SIZE];
  uint32_t compressedPixels;

  void initialize(Frame currentFrame, Frame previousFrame) {
    compressedPixels = 0;

    for (int i = 0; i < TOTAL_PIXELS; i++) {
      if (currentFrame.hasPixelChanged(i, previousFrame)) {
        setBit(temporal, i, true);
        compressedPixels++;
      } else
        setBit(temporal, i, false);
    }
  }

  bool hasPixelChanged(uint32_t pixelId) { return getBit(temporal, pixelId); }

 private:
  void setBit(uint8_t* bitarray, uint32_t n, bool value) {
    uint32_t byte = n / 8;
    uint8_t bit = n % 8;

    bitarray[byte] =
        value ? bitarray[byte] | (1 << bit) : bitarray[byte] & ~(1 << bit);
  }

  bool getBit(uint8_t* bitarray, uint32_t n) {
    uint32_t byte = n / 8;
    uint8_t bit = n % 8;

    return (bitarray[byte] >> bit) & 1;
  }
} ImageDiffBitArray;

#endif  // IMAGE_DIFF_BIT_ARRAY_H
