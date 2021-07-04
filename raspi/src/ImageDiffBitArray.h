#ifndef IMAGE_DIFF_BIT_ARRAY_H
#define IMAGE_DIFF_BIT_ARRAY_H

#include <stdint.h>
#include "Frame.h"
#include "Protocol.h"

typedef struct {
  uint32_t compressedPixels;
  uint8_t temporal[TEMPORAL_DIFF_SIZE];
  uint8_t paletteIndexByCompressedIndex[PALETTE_COLORS];
  uint8_t compressedIndexByPaletteIndex[PALETTE_COLORS];
  bool usedColorFlags[PALETTE_COLORS];
  uint32_t uniqueColors;
  uint32_t startPixel;

  void initialize(Frame currentFrame, Frame previousFrame) {
    compressedPixels = 0;
    uniqueColors = 0;
    startPixel = TOTAL_PIXELS;
    bool hasStartPixel = false;

    for (int i = 0; i < PALETTE_COLORS; i++) {
      usedColorFlags[i] = false;
      paletteIndexByCompressedIndex[i] = 0;
      compressedIndexByPaletteIndex[i] = 0;
    }

    for (int i = 0; i < TOTAL_PIXELS; i++) {
      if (!usedColorFlags[currentFrame.raw8BitPixels[i]]) {
        uint8_t colorIndex = currentFrame.raw8BitPixels[i];
        usedColorFlags[colorIndex] = true;
        paletteIndexByCompressedIndex[uniqueColors] = colorIndex;
        compressedIndexByPaletteIndex[colorIndex] = uniqueColors;
        uniqueColors++;
      }

      if (currentFrame.hasPixelChanged(i, previousFrame)) {
        if (!hasStartPixel) {
          startPixel = i;
          hasStartPixel = true;
        }
        setBit(temporal, i, true);
        compressedPixels++;
      } else
        setBit(temporal, i, false);
    }
  }

  bool isSpatialCompressed() { return uniqueColors < SPATIAL_DIFF_COLOR_LIMIT; }

  bool hasPixelChanged(uint32_t pixelId) { return getBit(temporal, pixelId); }

  bool isRepeatedColor(Frame frame, uint32_t pixelId) {
    return isSpatialCompressed() && pixelId < TOTAL_PIXELS &&
           hasPixelChanged(pixelId + 1) &&
           frame.raw8BitPixels[pixelId + 1] == frame.raw8BitPixels[pixelId];
  }

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
