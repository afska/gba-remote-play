#ifndef IMAGE_DIFF_RLE_COMPRESSOR_H
#define IMAGE_DIFF_RLE_COMPRESSOR_H

#include <stdint.h>
#include "Frame.h"
#include "Protocol.h"

typedef struct {
  uint8_t temporalDiffs[TEMPORAL_DIFF_MAX_SIZE];
  uint8_t compressedPixels[TOTAL_SCREEN_PIXELS];
  uint8_t runLengthEncoding[TOTAL_SCREEN_PIXELS];
  uint32_t temporalDiffEndPacket;
  uint32_t totalCompressedPixels;
  uint32_t repeatedPixels;
  uint32_t startPixel;
  int lastChangedPixelId = -1;

  void initialize(Frame currentFrame,
                  Frame previousFrame,
                  uint32_t diffThreshold,
                  uint32_t renderMode) {
    uint32_t totalPixels = RENDER_MODE_PIXELS[renderMode];
    uint32_t rleIndex = 0;

    totalCompressedPixels = repeatedPixels = 0;
    startPixel = totalPixels;
    temporalDiffEndPacket = TEMPORAL_DIFF_MAX_PACKETS;

    for (int i = 0; i < totalPixels; i++) {
      if (currentFrame.hasPixelChanged(i, previousFrame, diffThreshold)) {
        // (a pixel changed)
        if (totalCompressedPixels > 0) {
          if (currentFrame.raw8BitPixels[lastChangedPixelId] !=
                  currentFrame.raw8BitPixels[i] ||
              runLengthEncoding[rleIndex] == MAX_RLE) {
            // (the pixel has a new color)
            rleIndex++;
            runLengthEncoding[rleIndex] = 1;
          } else {
            // (the pixel has the same color as the last changed pixel)
            runLengthEncoding[rleIndex]++;
            repeatedPixels++;
          }
        } else {
          // (first changed pixel)
          startPixel = i;
          runLengthEncoding[0] = 1;
        }

        setBit(temporalDiffs, i, true);
        compressedPixels[totalCompressedPixels] = currentFrame.raw8BitPixels[i];
        totalCompressedPixels++;
        lastChangedPixelId = i;
      } else {
        // (a pixel remained with the same color as in the previous frame)
        setBit(temporalDiffs, i, false);
      }
    }

    if (lastChangedPixelId > -1) {
      // (detect buffer end to avoid sending useless bytes)
      temporalDiffEndPacket = (lastChangedPixelId / 8) / PACKET_SIZE + 1;
    }
  }

  uint32_t expectedPackets() {
    return size() / PIXELS_PER_PACKET + ((size() % PIXELS_PER_PACKET) != 0);
  }

  bool hasPixelChanged(uint32_t pixelId) {
    return getBit(temporalDiffs, pixelId);
  }

  uint32_t totalEncodedPixels() {
    return totalCompressedPixels - repeatedPixels;
  }

  bool shouldUseRLE() { return omittedRLEPixels() > 0; }
  int omittedRLEPixels() { return sizeWithoutRLE() - sizeWithRLE(); }
  uint32_t size() { return shouldUseRLE() ? sizeWithRLE() : sizeWithoutRLE(); }

 private:
  uint32_t sizeWithRLE() { return totalEncodedPixels() * 2; }
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
