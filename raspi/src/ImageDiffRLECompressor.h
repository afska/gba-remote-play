#ifndef IMAGE_DIFF_RLE_COMPRESSOR_H
#define IMAGE_DIFF_RLE_COMPRESSOR_H

#include <stdint.h>
#include "Frame.h"
#include "Protocol.h"

typedef struct {
  uint8_t temporalDiffs[TEMPORAL_DIFF_SIZE];
  uint8_t compressedPixels[TOTAL_PIXELS];
  uint8_t runLengthEncoding[TOTAL_PIXELS];
  uint32_t totalCompressedPixels;
  uint32_t rleIndex;
  uint32_t startPixel;
  int lastChangedPixelId = -1;

  void initialize(Frame currentFrame, Frame previousFrame) {
    totalCompressedPixels = rleIndex = 0;
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

        if (totalCompressedPixels > 0) {
          if (currentFrame.raw8BitPixels[lastChangedPixelId] !=
                  currentFrame.raw8BitPixels[i] ||
              runLengthEncoding[rleIndex] == 0xff) {
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

        setBit(temporalDiffs, i, true);
        compressedPixels[totalCompressedPixels] = currentFrame.raw8BitPixels[i];
        totalCompressedPixels++;
        lastChangedPixelId = i;
      } else {
        // (a pixel remained with the same color as in the previous frame)

        setBit(temporalDiffs, i, false);
      }
    }

#ifdef DEBUG
    if (rleIndex + 1 != totalCompressedPixels - repeatedPixels) {
      LOG("[!!!] RLE counters don't match (" + std::to_string(rleIndex + 1) +
          " vs " + std::to_string(totalCompressedPixels - repeatedPixels) +
          ")");
    }
#endif
  }

  uint32_t expectedPackets() {
    return size() / PIXELS_PER_PACKET + ((size() % PIXELS_PER_PACKET) != 0);
  }

  bool hasPixelChanged(uint32_t pixelId) {
    return getBit(temporalDiffs, pixelId);
  }

  bool shouldUseRLE() { return omittedRLEPixels() > 0; }
  int omittedRLEPixels() { return sizeWithoutRLE() - sizeWithRLE(); }
  uint32_t size() { return shouldUseRLE() ? sizeWithRLE() : sizeWithoutRLE(); }

 private:
  uint32_t sizeWithRLE() { return (rleIndex + 1) * 2; }
  uint32_t sizeWithoutRLE() { return totalCompressedPixels; }

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
} ImageDiffRLECompressor;

#endif  // IMAGE_DIFF_RLE_COMPRESSOR_H
