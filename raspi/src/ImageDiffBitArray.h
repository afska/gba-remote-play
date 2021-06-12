#ifndef IMAGE_DIFF_BIT_ARRAY_H
#define IMAGE_DIFF_BIT_ARRAY_H

#include <stdint.h>
#include "Frame.h"
#include "Protocol.h"

typedef struct {
  uint8_t temporal[TEMPORAL_DIFF_SIZE];
  uint8_t spatial[SPATIAL_DIFF_SIZE];
  uint32_t compressedPixels;

  void initialize(Frame currentFrame, Frame previousFrame) {
    uint32_t block = 0;
    uint32_t blockPart = 0;
    uint32_t omittedPixels = 0;
    bool hasDifferentColors = false;
    compressedPixels = 0;

    for (int i = 0; i < TOTAL_PIXELS; i++) {
      block = compressedPixels / SPATIAL_DIFF_BLOCK_SIZE;
      blockPart = compressedPixels % SPATIAL_DIFF_BLOCK_SIZE;

      if (currentFrame.hasPixelChanged(i, previousFrame)) {
        if (blockPart > 0) {
          hasDifferentColors =
              hasDifferentColors ||
              currentFrame.arePixelsDifferent(&currentFrame, &currentFrame,
                                              i - blockPart, i, false);

          if (blockPart == SPATIAL_DIFF_BLOCK_SIZE - 1) {
            setBit(spatial, block, hasDifferentColors);
            if (!hasDifferentColors)
              omittedPixels += SPATIAL_DIFF_BLOCK_SIZE - 1;
            hasDifferentColors = false;
          }
        }

        setBit(temporal, i, true);
        compressedPixels++;
      } else
        setBit(temporal, i, false);
    }

    compressedPixels -= omittedPixels;
  }

  bool hasPixelChanged(uint32_t pixelId) { return getBit(temporal, pixelId); }
  bool isRepeatedBlock(uint32_t blockId) { return !getBit(spatial, blockId); }

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
