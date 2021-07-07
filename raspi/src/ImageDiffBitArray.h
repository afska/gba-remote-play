#ifndef IMAGE_DIFF_BIT_ARRAY_H
#define IMAGE_DIFF_BIT_ARRAY_H

#include <stdint.h>
#include "Frame.h"
#include "Protocol.h"

typedef struct {
  uint8_t temporal[TEMPORAL_DIFF_SIZE];
  uint8_t runLengthEncoding[TOTAL_PIXELS];
  uint32_t compressedPixels;
  uint32_t rleIndex;
  uint32_t startPixel;
  int lastChangedPixelId = -1;

  void initialize(Frame currentFrame, Frame previousFrame) {
    compressedPixels = rleIndex = 0;
    startPixel = TOTAL_PIXELS;
    bool hasStartPixel = false;

#ifdef DEBUG
    uint32_t repeatedPixels = 0;
#endif

    for (int i = 0; i < TOTAL_PIXELS; i++) {
      if (currentFrame.hasPixelChanged(i, previousFrame)) {
        // (a pixel changed)

        if (!hasStartPixel) {
          startPixel = i;
          hasStartPixel = true;
        }

        if (compressedPixels > 0) {
          if (currentFrame.raw8BitPixels[lastChangedPixelId] !=
              currentFrame.raw8BitPixels[i]) {
            // (the pixel has a new color)

            rleIndex++;
            runLengthEncoding[rleIndex] = 1;
          } else {
            // (the pixel has the same color as the last changed pixel)

            runLengthEncoding[rleIndex]++;
#ifdef DEBUG
            repeatedPixels++;
#endif
          }
        } else
          runLengthEncoding[0] = 1;

        setBit(temporal, i, true);
        compressedPixels++;
        lastChangedPixelId = i;
      } else {
        // (a pixel remained with the same color as in the previous frame)

        setBit(temporal, i, false);
      }
    }

#ifdef DEBUG
    if (rleIndex + 1 != compressedPixels - repeatedPixels) {
      LOG("[!!!] RLE counters don't match");
    }
#endif
  }

  uint32_t expectedPackets() {
    return compressedPixels / PIXELS_PER_PACKET +
           ((compressedPixels % PIXELS_PER_PACKET) != 0);
  }

  uint32_t sizeWithRLE() { return (rleIndex + 1) * 2; }
  uint32_t sizeWithoutRLE() { return compressedPixels; }
  uint32_t omittedRLEPixels() { return sizeWithoutRLE() - sizeWithRLE(); }
  bool shouldUseRLE() { return sizeWithRLE() < sizeWithoutRLE(); }

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
