#ifndef IMAGE_DIFF_BIT_ARRAY_H
#define IMAGE_DIFF_BIT_ARRAY_H

#include <stdint.h>
#include "Frame.h"
#include "Protocol.h"

typedef struct {
  uint32_t compressedPixels;
  uint8_t temporal[DIFF_SIZE];
  uint8_t spatial[DIFF_SIZE];
  uint32_t startPixel;

  void initialize(Frame currentFrame, Frame previousFrame) {
    compressedPixels = 0;
    startPixel = TOTAL_PIXELS;
    bool hasStartPixel = false;

    for (int i = 0; i < TOTAL_PIXELS; i++) {
      if (currentFrame.hasPixelChanged(i, previousFrame)) {
        if (!hasStartPixel) {
          startPixel = i;
          hasStartPixel = true;
        }

        setBit(temporal, i, true);
        setBit(spatial, i,
               i < TOTAL_PIXELS - 1 && currentFrame.raw8BitPixels[i + 1] ==
                                           currentFrame.raw8BitPixels[i]);

        compressedPixels++;
      } else {
        setBit(temporal, i, false);
        if (i > 0)
          setBit(spatial, i - 1, false);
      }
    }
  }

  bool hasPixelChanged(uint32_t pixelId) { return getBit(temporal, pixelId); }
  bool isRepeatedColor(uint32_t pixelId) { return getBit(spatial, pixelId); }

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
