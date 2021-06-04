#ifndef TEMPORAL_DIFF_BIT_ARRAY_H
#define TEMPORAL_DIFF_BIT_ARRAY_H

#include <stdint.h>
#include "Frame.h"
#include "Protocol.h"

typedef struct {
  uint8_t data[TEMPORAL_DIFF_SIZE];

  void initialize(Frame currentFrame, Frame previousFrame) {
    for (int i = 0; i < TOTAL_PIXELS; i++)
      setBit(i, currentFrame.hasPixelChanged(i, previousFrame));
  }

 private:
  void setBit(uint32_t n, bool value) {
    uint32_t byte = n / 8;
    uint8_t bit = n % 8;

    data[byte] = value ? data[byte] | (1 << bit) : data[byte] & ~(1 << bit);
  }
} TemporalDiffBitArray;

#endif  // TEMPORAL_DIFF_BIT_ARRAY_H
